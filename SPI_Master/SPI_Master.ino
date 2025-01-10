#include <SPI.h>
#include "Config.h"

SPIClass master(VSPI);

#ifdef CONFIG_IDF_TARGET_ESP32
static constexpr uint8_t PIN_SSA = 5;  // CS untuk Slave A
static constexpr uint8_t PIN_SSB = 33; // CS untuk Slave B
#else
static constexpr uint8_t PIN_SSA = SSA;
static constexpr uint8_t PIN_SSB = SSB;
#endif

static constexpr size_t BUFFER_SIZE = 8;  // Buffer untuk data analog (2 byte)
uint8_t tx_buf[BUFFER_SIZE] {0};          // Buffer pengiriman
uint8_t rx_buf[BUFFER_SIZE] {0};          // Buffer penerimaan

// Daftar chip select untuk slave
uint8_t chipSelectPins[] = {PIN_SSA, PIN_SSB};

void setup() {
    Serial.begin(115200);
    delay(2000);

    for (uint8_t csPin : chipSelectPins) {
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);  // Pastikan slave tidak dipilih saat inisialisasi
    }

    master.begin(SCK, MISO, MOSI, SS);
    Serial.println("SPI Master initialized");
}

void communicateWithSlave(uint8_t csPin) {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(csPin, LOW);  // Pilih slave
    master.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    master.endTransaction();
    digitalWrite(csPin, HIGH);  // Lepaskan slave

    uint16_t analogValue = (rx_buf[0] << 8) | rx_buf[1];
    Serial.print("Analog value from Slave on pin ");
    Serial.print(csPin);
    Serial.print(": ");
    Serial.println(analogValue);
}

void loop() {
    for (uint8_t csPin : chipSelectPins) {
        communicateWithSlave(csPin);
        delay(10);  // Jeda untuk menghindari konflik bus
    }
    delay(50);  // Jeda sebelum siklus berikutnya
}
