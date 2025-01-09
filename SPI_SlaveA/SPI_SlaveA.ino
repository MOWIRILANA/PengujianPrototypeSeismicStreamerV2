#include "ESP32SPISlave_A.h"
#include "Config.h"

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 8;
uint8_t tx_buf[BUFFER_SIZE] {0};
uint8_t rx_buf[BUFFER_SIZE] {0};

void setup() {
    Serial.begin(115200);
    delay(2000);

    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(1);
    slave.begin();

    Serial.println("SPI Slave A initialized");
}

void loop() {
    uint16_t analogValue = analogRead(4);  // Baca dari pin analog lainnya
    tx_buf[0] = analogValue >> 8;           // High byte
    tx_buf[1] = analogValue & 0xFF;         // Low byte

    const size_t send_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

    // Debug: Tampilkan data yang dikirim dan diterima
    Serial.print("Analog value A sent: ");
    Serial.println((tx_buf[0] << 8) | tx_buf[1]);
    // Serial.print("Analog value A sent: ");
    // slave.transfer(tx_buf, rx_buf, 2);

    delay(100);
}
