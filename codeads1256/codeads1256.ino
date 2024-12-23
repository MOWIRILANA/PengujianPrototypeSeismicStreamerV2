#include <SPI.h>

#define CS_PIN 5     // Pin Chip Select
#define DRDY_PIN 2  // Pin Data Ready

void setup() {
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  digitalWrite(CS_PIN, HIGH); // Pastikan slave tidak aktif
  SPI.begin(8, 9, 10, CS_PIN);   // SCK, MISO, MOSI, SS

  Serial.begin(115200);
  ads1256Reset();
  ads1256Init();
}

void loop() {
  if (digitalRead(DRDY_PIN) == LOW) {  // Periksa apakah data siap
    long adcValue = ads1256ReadData();
    Serial.print("ADC Value: ");
    Serial.println(adcValue);
  }
}

// Fungsi untuk mereset ADS1256
void ads1256Reset() {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0xFE);  // Perintah reset
  digitalWrite(CS_PIN, HIGH);
  delay(1);            // Tunggu reset selesai
}

// Fungsi untuk inisialisasi registri ADS1256
void ads1256Init() {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x50);  // Perintah menulis ke register STATUS
  SPI.transfer(0x03);  // Konfigurasi STATUS register (contoh: Buffer aktif)
  digitalWrite(CS_PIN, HIGH);

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x51);  // Perintah menulis ke register MUX
  SPI.transfer(0x01);  // Konfigurasi MUX untuk channel diferensial (AIN0-AIN1)
  digitalWrite(CS_PIN, HIGH);
}

// Fungsi untuk membaca data dari ADS1256
long ads1256ReadData() {
  long adcValue = 0;
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x01);  // Perintah untuk Start Read Continuous Data
  delayMicroseconds(10);
  adcValue = SPI.transfer(0x00) << 16; // Baca byte pertama
  adcValue |= SPI.transfer(0x00) << 8; // Baca byte kedua
  adcValue |= SPI.transfer(0x00);      // Baca byte ketiga
  digitalWrite(CS_PIN, HIGH);

  if (adcValue & 0x800000) { // Periksa apakah negatif
    adcValue |= 0xFF000000; // Tanda ekstensi untuk data 24-bit
  }
  return adcValue;
}
