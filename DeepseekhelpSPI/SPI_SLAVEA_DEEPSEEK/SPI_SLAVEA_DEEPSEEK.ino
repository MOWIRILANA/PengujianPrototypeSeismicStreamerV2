#include <SPI.h>

const int analogPin = A4; // Pin analog untuk sensor

void setup() {
  // Inisialisasi SPI sebagai Slave
  pinMode(MISO, OUTPUT); // Set MISO sebagai output
  SPCR |= _BV(SPE); // Aktifkan SPI dalam mode Slave
  SPCR |= _BV(SPIE); // Aktifkan interrupt SPI
}

// Interrupt Service Routine (ISR) untuk SPI
ISR(SPI_STC_vect) {
  byte command = SPDR; // Baca perintah dari Master
  if (command == 0x01) { // Jika perintah adalah "kirim data"
    uint16_t analogValue = analogRead(analogPin); // Baca nilai analog
    SPDR = highByte(analogValue); // Kirim byte tinggi
    SPDR = lowByte(analogValue);  // Kirim byte rendah
  }
}

void loop() {
  // Tidak perlu melakukan apa-apa di loop utama
}