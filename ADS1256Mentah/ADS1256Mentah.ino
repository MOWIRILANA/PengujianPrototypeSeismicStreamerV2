#include <SPI.h>

// Pin Definitions
#define CS 5    
#define RDY 21  
#define RESET 22   // Add reset pin (choose any available pin)

// SPI Speed
#define SPISPEED 2500000  

void setup() {
  Serial.begin(115200);
  delay(500);

  // ADS1256 Initialization
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH); // Ensure CS is high to start
  pinMode(RDY, INPUT);
  pinMode(RESET, OUTPUT);  // Set RESET pin as OUTPUT
  digitalWrite(RESET, HIGH);  // Ensure RESET is high initially
  SPI.begin(18, 19, 23, CS);
  
  // Reset ADS1256
  resetADS1256();
  configureADS1256();
  Serial.println("ADS1256 initialized. Starting readings...");
}

void loop() {
  readsensorads();
  delay(1);
}

void readsensorads() {
  float voltageA0 = readSingleEndedChannel(0); 
  float voltageA1 = readSingleEndedChannel(1); 
  float voltageA2 = readSingleEndedChannel(2); 

  Serial.print(voltageA0);
  Serial.print(";");
  Serial.print(voltageA1);
  Serial.print(";");
  Serial.println(voltageA2);
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

// Function to reset ADS1256
void resetADS1256() {
  digitalWrite(RESET, LOW);  // Set RESET pin LOW to reset the ADS1256
  delay(10);                  // Wait for a short time
  digitalWrite(RESET, HIGH);  // Set RESET pin HIGH to complete the reset
  delay(10);                  // Wait to stabilize after reset
}
