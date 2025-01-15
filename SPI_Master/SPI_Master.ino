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
    
    communicateWithSlave(PIN_SSB);  // Komunikasi dengan Slave B
    analogValueB = (rx_buf[0] << 8) | rx_buf[1];  // Data dari Slave B
    
    // Tampilkan data ke serial monitor dalam format grafis
    printDataForPlotting(analogValueA, analogValueB);
    
    delay(50);  // Jeda sebelum siklus berikutnya
}

void communicateWithSlave(uint8_t csPin) {
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
    digitalWrite(csPin, LOW);
    master.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    master.transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    master.endTransaction();
    digitalWrite(csPin, HIGH);

    uint16_t analogValue = (rx_buf[0] << 8) | rx_buf[1];
    Serial.print("Analog value from Slave on pin ");
    Serial.print(csPin);
    Serial.print(": ");
    Serial.println(analogValue);
}

void printDataForPlotting(uint16_t analogValueA, uint16_t analogValueB) {
    Serial.print(analogValueA);  
    Serial.print(",");  
    Serial.println(analogValueB);  
}