#include <SPI.h>

// Pin Definitions
#define DE_PIN 4
#define RE_PIN 5
#define CS 25     // PIN CS diubah ke GPIO25
#define RDY 21
#define SPISPEED 2500000

// Buffer data
float sensorData[3];
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);  // Inisialisasi Serial Monitor
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);

  SPI.begin();
  configureADS1256();
  Serial.println("Slave siap dengan ADS1256");
}

void loop() {
  // Baca sensor secara kontinu
  readAllChannels();
  
  // Cek permintaan data dari master
  if (Serial2.available()) {
    while(Serial2.available()) Serial2.read();  // Bersihkan buffer
    
    // Kirim data terbaru
    sendSensorData();
  }
}

void readAllChannels() {
  static unsigned long lastRead = 0;
  if(millis() - lastRead >= 10) {  // Baca sensor setiap 10ms
    sensorData[0] = readSingleEndedChannel(0);
    sensorData[1] = readSingleEndedChannel(1);
    sensorData[2] = readSingleEndedChannel(2);
    
    // Debug print
    // Serial.print("A0: ");
    // Serial.print(sensorData[0], 4);
    // Serial.print(" | A1: ");
    // Serial.print(sensorData[1], 4);
    // Serial.print(" | A2: ");
    // Serial.println(sensorData[2], 4);
    
    lastRead = millis();
  }
}

void sendSensorData() {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
  
  // Konversi float ke byte array
  byte byteBuffer[12];
  memcpy(byteBuffer, &sensorData[0], 4);
  memcpy(byteBuffer+4, &sensorData[1], 4);
  memcpy(byteBuffer+8, &sensorData[2], 4);
  
  Serial2.write(byteBuffer, 12);
  Serial2.flush();
  
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
}

// Fungsi configureADS1256() dan readSingleEndedChannel() 
// tetap sama seperti contoh kode referensi

void configureADS1256() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);
  delay(2);
  
  // Soft reset
  SPI.transfer(0xFE);
  delay(50); // Tambahkan delay lebih lama
  
  // Kalibrasi offset dan gain
  SPI.transfer(0x62); // Offset calibration
  delay(100);
  SPI.transfer(0x61); // Gain calibration
  delay(100);
  
  writeRegister(0x00, 0x01); // STATUS: Analog input buffer disable
  writeRegister(0x02, 0x00); // ADCON: Gain 1, Sensor Detect OFF
  writeRegister(0x03, 0xF0); // DRATE: 30kSPS
  
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg);
  SPI.transfer(0x00);
  SPI.transfer(value);
  delayMicroseconds(100);
}

float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);
  
  // Wait for DRDY with timeout
  unsigned long start = micros();
  while(digitalRead(RDY) && (micros() - start) < 1000); // Timeout 1ms
  
  byte muxValue = (channel << 4) | 0x08;
  writeRegister(0x01, muxValue);
  
  SPI.transfer(0xFC); // SYNC
  delayMicroseconds(10); // Tambahkan delay lebih panjang
  SPI.transfer(0x00); // WAKEUP
  delayMicroseconds(10);
  
  SPI.transfer(0x01); // RDATA
  while(digitalRead(RDY)); // Tunggu sampai data ready
  
  // Baca 3 byte dengan proteksi timeout
  long rawValue = 0;
  for(int i=0; i<3; i++) {
    start = micros();
    while(!SPI.transfer(0) && (micros() - start) < 100); // Timeout 100μs per byte
    rawValue = (rawValue << 8) | SPI.transfer(0);
  }
  
  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  // Validasi nilai raw
  if(rawValue == 0xFFFFFF || rawValue == 0x000000) {
    Serial.println("ADC Overflow Detected!");
    return NAN;
  }
  
  if(rawValue & 0x800000) rawValue |= 0xFF000000;
  float voltage = rawValue * (3.3 / 0x7FFFFF);
  
  // Batasi nilai output
  if(voltage > 3.3 || voltage < -3.3) return NAN;
  
  return voltage;
}