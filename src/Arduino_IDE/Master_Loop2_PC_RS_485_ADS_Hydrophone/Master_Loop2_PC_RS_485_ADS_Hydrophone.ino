#include <SPI.h>
#include <ModbusRTU.h>

// Pin Definitions for ADS1256
#define CS 5    
#define RDY 21  
#define SPISPEED 2500000  

// Pin Definitions for RS485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4  

ModbusRTU mb;
float hydrophoneValue = 0.0; // Nilai dalam dB re 1 µPa
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 1000; // Debug print interval (1 second)

// Hydrophone constants (from datasheet)
const float SENSITIVITY_DB = -200.0; // Sensitivity in dB re 1 µPa
const float REFERENCE_VOLTAGE = 3.3; // ADS1256 reference voltage in volts
const float ADC_MAX_VALUE = 8388607.0; // Max ADC value (24-bit resolution)
const float SENSITIVITY_VOLTAGE = 0.0001; // Hydrophone sensitivity (V/µPa)

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  mb.begin(&Serial2, DE_RE_PIN); // Pass DE_RE pin to library
  mb.slave(1);

  // Add single Modbus holding register for hydrophone value (as float)
  mb.addHreg(0);
  Serial.println("Register 0 added for hydrophone value (dB re 1 µPa).");

  // Initialize SPI and ADS1256
  SPI.begin();
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(RDY, INPUT);

  configureADS1256();
  Serial.println("System initialized. Waiting for Modbus commands...");
}

void loop() {
  // Read analog value from ADS1256 channel 0
  float voltage = readSingleEndedChannel(0); // Voltage output of hydrophone

  // Convert voltage to pressure and then to dB re 1 µPa
  if (voltage >= 0) { // Ensure voltage is valid
    float pressurePa = voltage / SENSITIVITY_VOLTAGE; // Pressure in µPa
    hydrophoneValue = 20 * log10(pressurePa / 1.0); // Convert to dB re 1 µPa
  } else {
    hydrophoneValue = -999.0; // Indicate invalid reading
  }

  // Update Modbus register
  uint16_t modbusValue = (uint16_t)(hydrophoneValue * 10); // Store as scaled integer
  mb.Hreg(0, modbusValue);

  // Periodic debug output
  if (millis() - lastDebugTime >= DEBUG_INTERVAL) {
    Serial.println("\n=== Debug Info ===");
    Serial.printf("Raw Voltage: %.6f V\n", voltage);
    Serial.printf("Hydrophone Value (dB re 1 µPa): %.2f dB\n", hydrophoneValue);
    Serial.printf("Modbus Register Value: %d\n", modbusValue);
    Serial.println("================\n");
    lastDebugTime = millis();
  }

  // Handle Modbus communication
  mb.task();
  yield();
  delay(10);
}

// Function to configure ADS1256
void configureADS1256() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  delay(2);
  SPI.transfer(0xFE); // Reset command
  delay(5);

  // Set STATUS register (most significant bit first, buffer disabled)
  writeRegister(0x00, 0x01);

  // Set ADCON register (gain = 1, clock off)
  writeRegister(0x02, 0x00);

  // Set DRATE register (data rate = 30,000SPS)
  writeRegister(0x03, 0xF0);

  // Perform self-calibration
  SPI.transfer(0xF0);
  delay(400);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

// Function to write to a register
void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg); // Write command to register
  SPI.transfer(0x00);       // Write one byte
  SPI.transfer(value);      // Register value
  delayMicroseconds(100);
}

// Function to read single-ended channel
float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  // Wait for DRDY to go low
  while (digitalRead(RDY));

  // Set MUX register to select the desired channel (AINx-AINCOM)
  byte muxValue = (channel << 4) | 0x08; // AIN0 to AINCOM
  writeRegister(0x01, muxValue);

  // Send SYNC and WAKEUP commands
  SPI.transfer(0xFC); // SYNC command
  delayMicroseconds(2);
  SPI.transfer(0x00); // WAKEUP command
  delay(2);

  // Read ADC result
  SPI.transfer(0x01); // RDATA command
  delayMicroseconds(5);

  long rawValue = 0;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  // Convert raw value to voltage
  if (rawValue & 0x800000) {
    rawValue |= 0xFF000000; // Sign extend for negative values
  }

  float voltage = rawValue * (REFERENCE_VOLTAGE / ADC_MAX_VALUE); // Convert to voltage
  return voltage;
}
