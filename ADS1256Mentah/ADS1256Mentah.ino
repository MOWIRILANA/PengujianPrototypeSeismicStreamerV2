#include <SPI.h>

// Pin konfigurasi
#define CS_PIN 5        // Chip Select
#define DRDY_PIN 4      // Data Ready
#define SCLK_PIN 18     // SPI Clock
#define MISO_PIN 19     // SPI MISO
#define MOSI_PIN 23     // SPI MOSI

unsigned long previousMillis = 0;
unsigned long interval = 1000;
int dataCount = 0;

// Variabel global
volatile bool dataReady = false;  // Menandakan jika data siap dibaca

void IRAM_ATTR drdyInterrupt() {
  dataReady = true;  // Tandai bahwa data siap
  Serial.println("DRDY ok");
}

void sendCommand(byte command) {
  digitalWrite(CS_PIN, LOW);       // Aktifkan chip ADS1256
  SPI.transfer(command);           // Kirim perintah
  digitalWrite(CS_PIN, HIGH);      // Nonaktifkan chip
}

long readADCData() {
  digitalWrite(CS_PIN, LOW);       // Aktifkan chip ADS1256
  SPI.transfer(0x01);              // Kirim perintah RDATA (Read Data)
  delayMicroseconds(10);           // Tunggu sedikit waktu

  // Baca 3 byte data hasil konversi (24-bit)
  byte highByte = SPI.transfer(0x00);
  byte midByte = SPI.transfer(0x00);
  byte lowByte = SPI.transfer(0x00);

  digitalWrite(CS_PIN, HIGH);      // Nonaktifkan chip

  // Gabungkan 3 byte menjadi 24-bit signed value
  long result = (long(highByte) << 16) | (long(midByte) << 8) | long(lowByte);
  if (result & 0x800000) {         // Jika bit MSB adalah 1 (angka negatif)
    result |= 0xFF000000;          // Perluas menjadi 32-bit signed
  }

  return result;
}

void setup() {
  Serial.begin(115200);
  SPI.begin(SCLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(DRDY_PIN), drdyInterrupt, FALLING);

  digitalWrite(CS_PIN, HIGH);

  // Setup awal ADS1256
  sendCommand(0x06);  // RESET
  delay(100);
  sendCommand(0x08);  // Synchronized Reset
  delay(100);
  
  // Konfigurasi ADS1256
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x50); // WREG (Write Register), Start from MUX register
  SPI.transfer(0x03); // Number of bytes to write - 4 (MUX, STATUS, ADCCON, DRATE)
  SPI.transfer(0x08); // MUX: Channel AINP = AIN0, AINN = AINCOM (single-ended)
  SPI.transfer(0x01); // STATUS: Enable buffer
  SPI.transfer(0x00); // ADCCON: Gain = 1
  SPI.transfer(0xD0); // DRATE: 30 kSPS
  digitalWrite(CS_PIN, HIGH);

  delay(100);  // Tunggu hingga konfigurasi selesai
}

void loop() {
  if (dataReady) {
    dataReady = false;
    Serial.println("Ready ADS1256");

    // Baca data ADC
    long adcValue = readADCData();
    Serial.print("ADC,");
    Serial.println(adcValue);
  }
  dataCount++;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    Serial.print("Data received per second: ");
    Serial.println(dataCount);  // Tampilkan jumlah data per detik
    dataCount = 0;  // Reset counter
    previousMillis = currentMillis;  // Update waktu sebelumnya
  }
}
