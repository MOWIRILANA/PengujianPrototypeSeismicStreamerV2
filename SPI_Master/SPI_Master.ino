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

static constexpr size_t BUFFER_SIZE = 2;  // Sesuai dengan pengiriman data analog (2 byte)
uint8_t tx_buf[BUFFER_SIZE] {0};          // Buffer pengiriman
uint8_t rx_buf[BUFFER_SIZE] {0};          // Buffer penerimaan

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(PIN_SSA, OUTPUT);
    pinMode(PIN_SSB, OUTPUT);
    digitalWrite(PIN_SSA, HIGH);  // Deselect Slave A
    digitalWrite(PIN_SSB, HIGH);  // Deselect Slave B

    master.begin(SCK, MISO, MOSI, PIN_SSA);  // Inisialisasi SPI
    Serial.println("SPI Master initialized");
}

void loop() {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);

    master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SSA, LOW);  // Pilih Slave A
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(PIN_SSA, HIGH); // Deselect Slave A
    master.endTransaction();

    uint16_t analogValue = (rx_buf[0] << 8) | rx_buf[1];  // Kombinasikan dua byte menjadi satu nilai
    Serial.print("Received analog value from Slave A: ");
    Serial.println(analogValue);

    delay(500);  // Interval untuk pembacaan berikutnya
}
