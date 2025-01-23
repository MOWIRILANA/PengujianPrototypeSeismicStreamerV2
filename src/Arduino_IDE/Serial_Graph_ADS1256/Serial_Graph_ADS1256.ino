#include <SPI.h>

// Pin Definitions
#define CS 5    
#define RDY 21  
#define SPISPEED 2500000  

unsigned long previousMillis = 0;   // Menyimpan waktu sebelumnya untuk debug
unsigned long lastReadMillis = 0;  // Menyimpan waktu pembacaan terakhir
unsigned long sampleCount = 0;     // Hitungan jumlah sampel per detik

void setup() {
  Serial.begin(115200);
  SPI.begin();
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(RDY, INPUT);

  configureADS1256();
  Serial.println("System initialized. Starting readings...");
}

void loop() {
  unsigned long currentMillis = millis();

  // Baca data setiap 5 ms (200 sampel per detik)
  if (currentMillis - lastReadMillis >= 5) {
    readsensorads();
    sampleCount++;
    lastReadMillis = currentMillis;
  }

  // Tampilkan jumlah sampel per detik setiap 1 detik
  if (currentMillis - previousMillis >= 1000) {
    Serial.print("Samples per second: ");
    Serial.println(sampleCount);
    sampleCount = 0;  // Reset hitungan sampel
    previousMillis = currentMillis;
  }
}

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
  writeRegister(0x03, 0xF0);  // 30,000 SPS

  // Perform self-calibration
  SPI.transfer(0xF0);
  delay(400);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg); // Write command to register
  SPI.transfer(0x00);       // Write one byte
  SPI.transfer(value);      // Register value
  delayMicroseconds(100);
}

void readsensorads() {
  float voltageA0 = readSingleEndedChannel(0);
  float voltageA1 = readSingleEndedChannel(1);
  float voltageA2 = readSingleEndedChannel(2);

  // Cetak hasil pembacaan ke Serial Monitor
  Serial.print(voltageA0);
  Serial.print(";");
  Serial.print(voltageA1);
  Serial.print(";");
  Serial.println(voltageA2);
}

float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  // Tunggu hingga DRDY (data ready) rendah
  while (digitalRead(RDY));

  // Set MUX register untuk memilih channel
  byte muxValue = (channel << 4) | 0x08; // AINx-AINCOM
  writeRegister(0x01, muxValue);

  // Kirim perintah SYNC dan WAKEUP
  SPI.transfer(0xFC); // SYNC command
  delayMicroseconds(2);
  SPI.transfer(0x00); // WAKEUP command
  delay(2);

  // Baca hasil ADC
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

  // Konversi nilai mentah menjadi tegangan
  if (rawValue & 0x800000) { // Jika nilai negatif
    rawValue -= 16777216;    // Konversi ke dua's complement
  }

  // Konversi ke tegangan (asumsi referensi tegangan adalah 3.3V)
  float voltage = rawValue * (3.3 / 0x7FFFFF);
  return voltage;
}
