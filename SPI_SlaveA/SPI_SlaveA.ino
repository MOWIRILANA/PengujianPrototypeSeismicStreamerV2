#include "ESP32SPISlave_A.h"
#include <SPI.h>
#include "Config.h"

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 8;
uint8_t tx_buf[BUFFER_SIZE] {1, 2, 3, 4, 5, 6, 7, 8};
uint8_t rx_buf[BUFFER_SIZE] {0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
    Serial.begin(115200);
    delay(2000);

    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(1);
    slave.begin();
    Serial.print("SS:");
    Serial.println(SS);
    Serial.println("SPI Slave 22 initialized");
}

void loop() {
    readanalog();
    const size_t send_bytes = slave.transfer(tx_buf, NULL, BUFFER_SIZE);
    const std::vector<size_t> received_bytes = slave.wait();

    Serial.print("Analog value 22 sent: ");
    Serial.println((tx_buf[0] << 8) | tx_buf[1]);

    for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(tx_buf[i], HEX);
    Serial.print(" ");
    }
    Serial.println();
    delay(10);
}

void readanalog(){
  uint16_t analogValue = analogRead(4);  
    tx_buf[0] = analogValue >> 8;
    tx_buf[1] = analogValue & 0xFF;
}