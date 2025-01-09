#include <SPI.h>
#include "Config.h"

SPIClass master(VSPI);
#ifdef CONFIG_IDF_TARGET_ESP32
static constexpr uint8_t PIN_SSA = 5;
static constexpr uint8_t PIN_SSB = 33;
#else
static constexpr uint8_t PIN_SSA = SSA;
static constexpr uint8_t PIN_SSB = SSB;
#endif

static constexpr size_t BUFFER_SIZE = 8;
uint8_t tx_buf[BUFFER_SIZE] {0};
uint8_t rx_buf[BUFFER_SIZE] {0};

void setup()
{
    Serial.begin(115200);

    delay(2000);

    pinMode(PIN_SSA, OUTPUT);
    pinMode(PIN_SSB, OUTPUT);
    digitalWrite(PIN_SSA, HIGH);
    digitalWrite(PIN_SSB, HIGH);

    // master.begin(SCK, MISO, MOSI, PIN_SSA);
    // Serial.println("start spi masterA");

    master.begin(SCK, MISO, MOSI, PIN_SSA);
    Serial.println("start spi masterA");
    master.begin(SCK, MISO, MOSI, PIN_SSB);
    Serial.println("start spi masterB");
    delay(2000);

}

void loop()
{
    // initialize tx/rx buffers
    initializeBuffers(rx_buf, 0, BUFFER_SIZE);

    // Transfer data ke Slave A
    digitalWrite(PIN_SSA, LOW);
    master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    master.endTransaction();
    digitalWrite(PIN_SSA, HIGH);

    // Tampilkan data dari Slave A
    Serial.print("Data dari Slave A: ");
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(rx_buf[i]);
        if (i < BUFFER_SIZE - 1) Serial.print(", ");
    }
    Serial.println();

    // Reset buffer penerima
    // memset(rx_buf, 0, BUFFER_SIZE);
    // initializeBuffers(rx_buf, 0, BUFFER_SIZE);

    // // Setup Menerima Data slave B
    // master.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    // digitalWrite(PIN_SSB, LOW);
    // master.transferBytes(rx_buf, tx_buf, BUFFER_SIZE);
    // digitalWrite(PIN_SSB, HIGH);
    // master.endTransaction();

    // // Menampilkan data yang diterima
    // Serial.print("Data dari Slave B: ");
    // for (size_t i = 0; i < BUFFER_SIZE; i++) {
    //     Serial.print(rx_buf[i]);
    //     if (i < BUFFER_SIZE - 1) Serial.print(", ");
    // }
    // Serial.println();

}

void verifikasi(){
  // verify and dump difference with received data
    if (verifyAndDumpDifference("master", rx_buf, BUFFER_SIZE, "slaveB", tx_buf, BUFFER_SIZE)) {
        // Serial.println(rx_buf);
        Serial.println("Pesan Diterima");
    } else {
        Serial.println("unexpected difference found between master/slave data");
    }
}