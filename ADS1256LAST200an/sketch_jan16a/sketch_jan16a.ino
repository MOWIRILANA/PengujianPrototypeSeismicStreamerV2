#include <SPI.h>

// Pin Definitions
#define CS_SLAVE_A 2  // Chip Select untuk Slave A
#define CS_SLAVE_B 22 // Chip Select untuk Slave B

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Inisialisasi pin CS
    pinMode(CS_SLAVE_A, OUTPUT);
    pinMode(CS_SLAVE_B, OUTPUT);
    digitalWrite(CS_SLAVE_A, HIGH); // Nonaktifkan Slave A
    digitalWrite(CS_SLAVE_B, HIGH); // Nonaktifkan Slave B

    // Inisialisasi SPI
    SPI.begin();
    Serial.println("SPI Master initialized.");
}

void loop() {
    uint8_t dataFromSlaveA = communicateWithSlave(CS_SLAVE_A);
    uint8_t dataFromSlaveB = communicateWithSlave(CS_SLAVE_B);

    Serial.print("Data from Slave A: ");
    Serial.println(dataFromSlaveA);

    Serial.print("Data from Slave B: ");
    Serial.println(dataFromSlaveB);

    delay(100); // Jeda sebelum siklus berikutnya
}

uint8_t communicateWithSlave(uint8_t csPin) {
    digitalWrite(csPin, LOW); // Pilih slave
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Konfigurasi SPI
    uint8_t data = SPI.transfer(0x00); // Kirim dan terima data
    SPI.endTransaction();
    digitalWrite(csPin, HIGH); // Lepaskan slave
    return data;
}