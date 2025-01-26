#include <SPI.h>
#include "Config.h"

SPIClass master(VSPI);

#ifdef CONFIG_IDF_TARGET_ESP32
static constexpr uint8_t PIN_SSA = 2;  // CS untuk Slave A
static constexpr uint8_t PIN_SSB = 22; // CS untuk Slave B
#else
static constexpr uint8_t PIN_SSA = SSA;
static constexpr uint8_t PIN_SSB = SSB;
#endif

static constexpr size_t BUFFER_SIZE = 8;
uint8_t tx_buf[BUFFER_SIZE] {0};          
uint8_t rx_buf[BUFFER_SIZE] {0};          

uint8_t chipSelectPins[] = {PIN_SSA, PIN_SSB};

void setup() {
    Serial.begin(115200);
    delay(2000);

    for (uint8_t csPin : chipSelectPins) {
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
    }

    master.begin(SCK, MISO, MOSI, SS);
    Serial.println("SPI Master initialized");
}

void loop() {
    uint16_t analogValueA, analogValueB;

    communicateWithSlave(PIN_SSA);  // Komunikasi dengan Slave A
    analogValueA = (rx_buf[0] << 8) | rx_buf[1];  // Data dari Slave A
    digitalWrite(PIN_SSA, LOW);
    delay(10);
    communicateWithSlave(PIN_SSB);  // Komunikasi dengan Slave B
    analogValueB = (rx_buf[0] << 8) | rx_buf[1];  // Data dari Slave B
    digitalWrite(PIN_SSB, LOW);
    delay(10);
    
    delay(50);  // Jeda sebelum siklus berikutnya
}

void communicateWithSlave(uint8_t csPin) {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(csPin, LOW);
    master.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);

    uint16_t analogValue = (rx_buf[0] << 8) | rx_buf[1];
    Serial.print("Analog value from Slave on pin ");
    Serial.print(csPin);
    Serial.print(": ");
    Serial.println(analogValue);

    bool isValid = (analogValue >= 0 && analogValue <= 4095);
    uint8_t ack = isValid ? 0x01 : 0x00;  // 0x01 untuk ACK jika data valid, 0x00 untuk NACK jika data tidak valid
    master.transfer(ack);

    master.endTransaction();
    digitalWrite(csPin, HIGH);

    

    // Jika data tidak valid, Master bisa mengulang permintaan data
    if (!isValid) {
        Serial.println("Data invalid, requesting again...");
        // Anda bisa menambahkan logika untuk mengulang pengambilan data jika perlu
    }
    
}