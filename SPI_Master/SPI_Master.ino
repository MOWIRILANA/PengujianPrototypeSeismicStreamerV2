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

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(PIN_SSA, OUTPUT);
    pinMode(PIN_SSB, OUTPUT);
    digitalWrite(PIN_SSA, HIGH);
    digitalWrite(PIN_SSB, HIGH);

    master.begin(SCK, MISO, MOSI, SS);
    Serial.println("SPI Master initialized");
}

void loop() {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);

    // Komunikasi dengan Slave A
    digitalWrite(PIN_SSB, HIGH);
    master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SSA, LOW);
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(PIN_SSA, HIGH);
    master.endTransaction();
    uint16_t analogValueA = (rx_buf[0] << 8) | rx_buf[1];
    Serial.print("Analog value from Slave A: ");
    Serial.println(analogValueA);

    delay(10);  // Jeda untuk menghindari konflik bus
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);

    // Komunikasi dengan Slave B
    digitalWrite(PIN_SSA, HIGH);
    master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SSB, LOW);
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(PIN_SSB, HIGH);
    master.endTransaction();
    uint16_t analogValueB = (rx_buf[0] << 8) | rx_buf[1];
    Serial.print("Analog value from Slave B: ");
    Serial.println(analogValueB);

    delay(50);  // Jeda sebelum membaca lagi
}
