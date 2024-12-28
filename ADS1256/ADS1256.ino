#include <SPI.h>
#include <ADS1256_M.h>

// Pin Definitions
#define CS_PIN 5       // Chip Select
#define DRDY_PIN 21    // Data Ready
#define RESET_PIN 25   // Reset Pin

// SPI Clock Speed
#define SPI_SPEED 2000000 // 2 MHz

// Initialize ADS1256
void initADS1256() {
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  pinMode(RESET_PIN, OUTPUT);
  
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);

  // Reset ADS1256 using RESET pin
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(100); // Wait for reset to complete

  // Send Reset Command (0xFE)
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0xFE); // RESET command
  digitalWrite(CS_PIN, HIGH);

  delay(100); // Wait for reset to complete

  // Set Data Rate to 7500 SPS (DRATE Register 0x03 = 0xF0)
  writeRegister(0x03, 0xF0);
}

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, CS_PIN); // SCLK, MISO, MOSI, CS
  SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE1));

  initADS1256();
}

void loop() {
  for (uint8_t channel = 0; channel < 8; channel++) {
    // Set channel to single-ended mode (AINx vs AINCOM)
    setChannel(channel, 8); // AINx positive, AINCOM negative

    delay(10); // Allow MUX to stabilize

    // Wait for DRDY to go low (data ready)
    if (digitalRead(DRDY_PIN) == LOW) {
      uint32_t adcValue = readData();

      // Print ADC value for the current channel
      Serial.print("AIN");
      Serial.print(channel);
      Serial.print(": ");
      Serial.println(adcValue);
    }
  }

  delay(500); // Delay for readability
}

void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x50 | reg); // Write command
  SPI.transfer(0x00);      // Single-byte register
  SPI.transfer(value);     // Value to write
  digitalWrite(CS_PIN, HIGH);
}

void setChannel(uint8_t positive, uint8_t negative) {
  uint8_t muxValue = (positive << 4) | (negative & 0x0F);

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x50 | 0x01); // Write to MUX register
  SPI.transfer(0x00);        // Single-byte register
  SPI.transfer(muxValue);    // MUX value
  digitalWrite(CS_PIN, HIGH);

  // Send SYNC command
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0xFC); // SYNC command
  digitalWrite(CS_PIN, HIGH);

  // Send WAKEUP command
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x00); // WAKEUP command
  digitalWrite(CS_PIN, HIGH);
}

uint32_t readData() {
  uint32_t data = 0;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x01); // Send RDATA command
  delayMicroseconds(10);

  for (int i = 0; i < 3; i++) {
    data = (data << 8) | SPI.transfer(0x00); // Read 24 bits
  }

  digitalWrite(CS_PIN, HIGH);

  // Handle sign extension for 24-bit values
  if (data & 0x800000) {
    data |= 0xFF000000; // Negative number
  }

  return data;
}
