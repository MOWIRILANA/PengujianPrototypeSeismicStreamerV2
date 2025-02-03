#define TX_PIN 16
#define RX_PIN 17
#define led 2
#define USB_TX_PIN 4
#define USB_RX_PIN 5

void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    Serial2.begin(115200, SERIAL_8N1, TX_PIN, RX_PIN);  // RS485 UART
    Serial1.begin(115200, SERIAL_8N1, TX_PIN, USB_RX_PIN); // USB RS485 Full Duplex

    Serial.println("RS485 Master Ready. Waiting for data...");
}

void loop() {
    if (Serial2.available()) {
        digitalWrite(led, HIGH);
        String receivedData = Serial2.readStringUntil('\n');
        Serial.println("Received: " + receivedData);
        Serial1.println(receivedData); // Forward data to USB RS485
        }
    else{
      digitalWrite(led, LOW);
    }
}
