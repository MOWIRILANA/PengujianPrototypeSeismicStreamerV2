#include "ESP32SPISlave_B.h"
#include "Config.h"

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 8;  // Ukuran buffer
static constexpr size_t QUEUE_SIZE = 1;   // Ukuran antrian
uint8_t tx_buf[BUFFER_SIZE] {0};          // Buffer pengiriman
uint8_t rx_buf[BUFFER_SIZE] {0};          // Buffer penerimaan

void initializeBuffers(uint8_t *tx, uint8_t *rx, size_t size) {
    for (size_t i = 0; i < size; i++) {
        tx[i] = 0;  // Inisialisasi buffer pengiriman
        rx[i] = 0;  // Inisialisasi buffer penerimaan
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    slave.setDataMode(SPI_MODE0);   // Default: SPI_MODE0
    slave.setQueueSize(QUEUE_SIZE); // Default: 1
    slave.begin();

    Serial.println("SPI Slave initialized");
}

void loop() {

    uint16_t analogValue = analogRead(4);  // Baca nilai analog dari pin 34
    tx_buf[0] = analogValue >> 8;           // High byte
    tx_buf[1] = analogValue & 0xFF;         // Low byte

    const size_t send_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

    // Debug: Tampilkan data yang dikirim dan diterima
    Serial.print("Analog value B sent: ");
    Serial.println((tx_buf[0] << 8) | tx_buf[1]);
    
    delay(100);  // Interval waktu untuk pembacaan berikutnya
}
