#define TX_PIN 16
#define RX_PIN 17
#define led 2

void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    Serial2.begin(115200, SERIAL_8N1, TX_PIN, RX_PIN);  // RS485 UART

    Serial.println("RS485 Master Ready. Waiting for data...");
}

void loop() {
    if (Serial2.available()) {
        digitalWrite(led, HIGH);
        String receivedData = Serial2.readStringUntil('\n');
        Serial.println("Received: " + receivedData);
        }
    else{
      digitalWrite(led, LOW);
    }
}
