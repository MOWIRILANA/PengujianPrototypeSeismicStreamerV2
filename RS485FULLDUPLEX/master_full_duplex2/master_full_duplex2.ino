#define DE_PIN 4
#define RE_PIN 5

unsigned long lastRequest = 0;
unsigned long lastSumTime = 0;
float receivedData[3];
float sumData[3] = {0, 0, 0};
int countSamples = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
  
  Serial.println("Master siap");
}

void loop() {
  if (micros() - lastRequest >= 1000) { // Request setiap 1ms
    sendRequest();
    receiveData();
    lastRequest = micros();
  }

  if (millis() - lastSumTime >= 1000) {
    Serial.print("Total Samples dalam 1 detik: ");
    Serial.println(countSamples);
    
    sumData[0] = sumData[1] = sumData[2] = 0;
    countSamples = 0;
    lastSumTime = millis();
  }
}

void sendRequest() {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
  Serial2.write('R');
  Serial2.flush();
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
}

void receiveData() {
  byte buffer[12];
  int bytesRead = 0;
  unsigned long startTime = micros();
  
  while (bytesRead < 12 && (micros() - startTime) < 5000) { // Timeout 5ms
    if (Serial2.available()) {
      buffer[bytesRead++] = Serial2.read();
    }
  }
  
  if (bytesRead == 12) {
    memcpy(&receivedData[0], buffer, 12);
    
    // Tampilkan data ke Serial Monitor
    Serial.print("Diterima => ");
    Serial.print("A0: ");
    Serial.print(receivedData[0], 4);
    Serial.print(" | A1: ");
    Serial.print(receivedData[1], 4);
    Serial.print(" | A2: ");
    Serial.print(receivedData[2], 4);
    Serial.print(" | Seq: ");
    Serial.println(countSamples + 1);

    sumData[0] += receivedData[0];
    sumData[1] += receivedData[1];
    sumData[2] += receivedData[2];
    countSamples++;
  } else if (bytesRead > 0) {
    Serial.println("Error: Data tidak lengkap");
  }
}