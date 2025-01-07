#include <SPI.h>
#define led 2
#define VSPI 3

const int VSPI_MISO = 19;
const int VSPI_MOSI = 23;
const int VSPI_SCLK = 18;
const int VSPI_SS = 5;

SPIClass vspi = SPIClass(VSPI);

void setup() {
  Serial.begin(115200);
  pinMode(MISO, OUTPUT);  // Set jalur MISO sebagai output
  vspi.begin();            // Inisialisasi SPI sebagai slave
}

void loop() {
  byte dataReceived = SPI.transfer(0x00);  // Slave membaca data
  if (dataReceived == 0xAA) {
    Serial.println("Data 0xAA diterima!");
  }
  delay(100);
}
