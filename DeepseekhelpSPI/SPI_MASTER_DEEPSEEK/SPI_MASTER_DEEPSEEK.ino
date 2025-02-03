#include <SPI.h>

const int SS1 = 22; // Pin SS untuk Slave 1
const int SS2 = 2;  // Pin SS untuk Slave 2

void setup() {
  // Inisialisasi SPI sebagai Master
  SPI.begin(); // Mulai komunikasi SPI
  pinMode(SS1, OUTPUT); // Set pin SS1 sebagai output
  pinMode(SS2, OUTPUT); // Set pin SS2 sebagai output
  digitalWrite(SS1, HIGH); // Non-aktifkan Slave 1
  digitalWrite(SS2, HIGH); // Non-aktifkan Slave 2
  Serial.begin(9600); // Untuk debugging
}

void loop() {
  // Minta data dari Slave 1
  digitalWrite(SS1, LOW); // Aktifkan Slave 1
  byte requestData = 0x01; // Perintah untuk meminta data
  SPI.transfer(requestData); // Kirim perintah ke Slave 1
  uint16_t dataFromSlave1 = SPI.transfer16(0x00); // Terima data 16-bit dari Slave 1
  digitalWrite(SS1, HIGH); // Non-aktifkan Slave 1

  // Minta data dari Slave 2
  digitalWrite(SS2, LOW); // Aktifkan Slave 2
  SPI.transfer(requestData); // Kirim perintah ke Slave 2
  uint16_t dataFromSlave2 = SPI.transfer16(0x00); // Terima data 16-bit dari Slave 2
  digitalWrite(SS2, HIGH); // Non-aktifkan Slave 2

  // Tampilkan data yang diterima di Serial Monitor
  Serial.print("Data dari Slave 1: ");
  Serial.println(dataFromSlave1);
  Serial.print("Data dari Slave 2: ");
  Serial.println(dataFromSlave2);

  delay(1000); // Tunggu 1 detik sebelum meminta data lagi
}