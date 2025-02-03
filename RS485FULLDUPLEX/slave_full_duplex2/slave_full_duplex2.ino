#include <SPI.h>

// Pin Definitions
#define DE_PIN 4    // Data Enable for RS485
#define RE_PIN 5    // Receive Enable for RS485
#define CS 25       // Chip Select for ADS1256
#define RDY 21      // Data Ready for ADS1256

#define SPISPEED 2500000  // SPI speed for communication with ADS1256

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // Initialize Serial2 for RS485 communication

  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH); // Ensure CS is high to start
  digitalWrite(DE_PIN, LOW); // DE and RE low for receive mode
  digitalWrite(RE_PIN, LOW);

  pinMode(RDY, INPUT);

  SPI.begin(18, 19, 23, CS);  // Initialize SPI

  // Initialize ADS1256
  configureADS1256();
  Serial.println("ADS1256 initialized. Starting readings...");
}

void loop() {
  Serial.println("start loop");
  if (Serial2.available() && Serial2.read() == 'R') {
    // Read analog values from A0, A1, and A2
    float voltageA0 = readSingleEndedChannel(0); // Channel 0 corresponds to AIN0
    float voltageA1 = readSingleEndedChannel(1); // Channel 1 corresponds to AIN1
    float voltageA2 = readSingleEndedChannel(2); // Channel 2 corresponds to AIN2

    // Print the values to Serial Monitor
    Serial.print("A0: ");
    Serial.print(voltageA0, 6);
    Serial.print(" | A1: ");
    Serial.print(voltageA1, 6);
    Serial.print(" | A2: ");
    Serial.println(voltageA2, 6);

    // Send the data back to the master
    sendDataToMaster(voltageA0, voltageA1, voltageA2);
  }
}

// Function to send data to the master via RS485
void sendDataToMaster(float data0, float data1, float data2) {
  digitalWrite(DE_PIN, HIGH); // Enable transmission
  digitalWrite(RE_PIN, HIGH);
  delayMicroseconds(50);

  byte byteBuffer[12];
  float data[3] = {data0, data1, data2};
  memcpy(byteBuffer, data, 12);

  Serial2.write(byteBuffer, 12); // Send data
  Serial2.flush();

  digitalWrite(DE_PIN, LOW); // Disable transmission
  digitalWrite(RE_PIN, LOW);
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

  // Wait for DRDY to go low (with timeout)
  unsigned long startTime = micros();
  while (digitalRead(RDY) && (micros() - startTime) < 100000); // Timeout after 100ms

  // Set MUX register to select the desired channel (AINx-AINCOM)
  byte muxValue = (channel << 4) | 0x08; // Channel 0 = AIN0, Channel 1 = AIN1, Channel 2 = AIN2
  writeRegister(0x01, muxValue);

  // Send SYNC and WAKEUP commands
  SPI.transfer(0xFC); // SYNC command
  delayMicroseconds(2);
  SPI.transfer(0x00); // WAKEUP command
  delayMicroseconds(2);

  // Read ADC result
  SPI.transfer(0x01); // RDATA command
  delayMicroseconds(5);

  long rawValue = 0;
  rawValue |= SPI.transfer(0) << 16;
  rawValue |= SPI.transfer(0) << 8;
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