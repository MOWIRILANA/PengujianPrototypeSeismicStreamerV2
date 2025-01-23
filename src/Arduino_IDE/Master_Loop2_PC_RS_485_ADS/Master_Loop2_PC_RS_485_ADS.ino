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
uint16_t analogValues[3] = {0, 0, 0};
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 0; // Debug print interval (1 second)

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  mb.begin(&Serial2, DE_RE_PIN); // Pass DE_RE pin to library
  mb.slave(2);

  // Add Modbus holding registers with debug message
  for (int i = 0; i < 3; i++) {
    mb.addHreg(i);
    Serial.printf("Register %d added with initial value: %d\n", i, analogValues[i]);
  }

  // Initialize SPI and ADS1256
  SPI.begin();
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(RDY, INPUT);

  configureADS1256();
  Serial.println("System initialized. Waiting for Modbus commands...");
}

void loop() {
  // Read analog values from ADS1256
  analogValues[0] = (uint16_t)(readSingleEndedChannel(0) * 1000);
  analogValues[1] = (uint16_t)(readSingleEndedChannel(1) * 1000);
  analogValues[2] = (uint16_t)(readSingleEndedChannel(2) * 1000);

  // Update Modbus registers
  for (int i = 0; i < 3; i++) {
    mb.Hreg(i, analogValues[i]);
  }

  // Periodic debug output
  if (millis() - lastDebugTime >= DEBUG_INTERVAL) {
    Serial.println("\n=== Debug Info ===");
    Serial.printf("Modbus Status: Active on address %d\n", mb.slave());
    Serial.println("Register Values:");
    for (int i = 0; i < 3; i++) {
      Serial.printf("Reg[%d] = %d mV\n", i, mb.Hreg(i));
    }
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
  byte muxValue = (channel << 4) | 0x08; // Channel 0 = AIN0, Channel 1 = AIN1, Channel 2 = AIN2
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

  float voltage = rawValue * (3.3 / (0x7FFFFF)); // Assuming reference voltage is 3.3V
  return voltage;
}