#include <SPI.h>

const int CS_PIN = 5; // Chip Select
const int ANALOG_PIN = 4; // Pin analog untuk membaca data

void setup() {
  pinMode(CS_PIN, OUTPUT);
  SPI.begin();
  Serial.begin(115200);
}

void loop() {
  int analogValue = analogRead(ANALOG_PIN); // Baca nilai analog
  digitalWrite(CS_PIN, LOW); // Pilih slave
  SPI.transfer(analogValue >> 8); // Kirim byte tinggi
  SPI.transfer(analogValue & 0xFF); // Kirim byte rendah
  digitalWrite(CS_PIN, HIGH); // Lepaskan slave

  Serial.println(analogValue); // Tampilkan nilai analog di Serial Monitor
  delay(1000); // Tunggu 1 detik
}
