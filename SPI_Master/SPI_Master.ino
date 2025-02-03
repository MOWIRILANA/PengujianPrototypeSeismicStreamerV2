#include <SPI.h>
#include "Config.h"
#define led 2

SPIClass master(VSPI);

#ifdef CONFIG_IDF_TARGET_ESP32
static constexpr uint8_t PIN_SSA = 2;  // CS untuk Slave A
static constexpr uint8_t PIN_SSB = 22; // CS untuk Slave B
#else
static constexpr uint8_t PIN_SSA = SSA;
static constexpr uint8_t PIN_SSB = SSB;
#endif

static constexpr size_t BUFFER_SIZE = 8;
uint8_t tx_buf[BUFFER_SIZE] {1, 2, 3, 4, 5, 6, 7, 8};          
uint8_t rx_buf[BUFFER_SIZE] {0, 0, 0, 0, 0, 0, 0, 0};          

uint8_t chipSelectPins[] = {PIN_SSA, PIN_SSB};

void setup() {
    Serial.begin(115200);
    delay(2000);
    pinMode(led, OUTPUT);

    for (uint8_t csPin : chipSelectPins) {
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
    }

    master.begin(SCK, MISO, MOSI, PIN_SSA);
    Serial.print("SS:");
    Serial.println(PIN_SSA);
    Serial.println("SPI Master initialized");
}

void loop() {
    uint16_t analogValueA, analogValueB;

    communicateWithSlave(PIN_SSA);  // Komunikasi dengan Slave A
    analogValueA = (rx_buf[0] << 8) | rx_buf[1];  // Data dari Slave A
    // digitalWrite(PIN_SSA, LOW);
    // delay(10);
    digitalWrite(led, HIGH);
    delay(10);
    communicateWithSlave(PIN_SSB);  // Komunikasi dengan Slave B
    analogValueB = (rx_buf[0] << 8) | rx_buf[1];  // Data dari Slave B
    // digitalWrite(PIN_SSB, LOW);
    // delay(10);
    digitalWrite(led, LOW);
    delay(50);  // Jeda sebelum siklus berikutnya
}

void communicateWithSlave(uint8_t csPin) {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(csPin, LOW);
    master.beginTransaction(SPISettings(5000, MSBFIRST, SPI_MODE0));
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);

    uint16_t analogValue = (rx_buf[0] << 8) | rx_buf[1];
    for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(rx_buf[i], HEX);
    Serial.print(" ");
    }
    Serial.println();
    Serial.print("Analog value from Slave on pin");
    Serial.print(csPin);
    Serial.print(": ");
    Serial.println(analogValue);
    
    master.endTransaction();
    digitalWrite(csPin, HIGH);
    
}