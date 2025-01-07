// Slave B CODE

#include <SPI.h>

void setup() {
  Serial.begin(115200);
  SPI.begin(); // Start SPI
  pinMode(MISO, OUTPUT); // Set MISO for slave output
  SPCR |= _BV(SPE);      // Enable SPI in slave mode
}

void loop() {
  if (SPI.available()) {
    byte data = SPI.transfer(0); // Read incoming data
    Serial.print("Received data: ");
    Serial.println(data, HEX);
  }
}
