#include <SPI.h>
#include <Ethernet.h>
#include "local_config.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(169, 254, 61, 34);

EthernetServer server(80);

void WizReset() { 
    Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
    pinMode(RESET_P, OUTPUT);
    digitalWrite(RESET_P, HIGH);
    delay(250);
    digitalWrite(RESET_P, LOW);
    delay(50);
    digitalWrite(RESET_P, HIGH);
    delay(350);
    Serial.println("Done.");
}

void cableconnected(){
  byte phycfgr = readRegister(0x2E); // Address PHYCFGR = 0x2E
  bool linkStatus = phycfgr & 0x10; // Bit 4 = LINK
  
  if (linkStatus) {
    Serial.println("Ethernet cable not connected.");
  } else {
    Serial.println("Ethernet cable connected.");
  }
  delay(1000);
}

void hardwarecheck(){
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
}

void setup() {
  pinMode(w5500_cs, OUTPUT);
  digitalWrite(w5500_cs, HIGH);
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Example");
  Ethernet.init(w5500_cs);
  WizReset();
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  hardwarecheck();
  cableconnected();
  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 60");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<main id=\"main\" class=\"main-content\">"); //add some titles and contect classifications
          /*build the table****************************/
          client.println("<TABLE BORDER=\"5\" WIDTH=\"50%\" CELLPADDING=\"4\" CELLSPACING=\"3\">");
          client.println("<TR>");
          client.println("<TH COLSPAN=\"2\"><BR><H3>SENSOR DATA</H3>");
          client.println("</TH>");
          client.println("</TR>");
          client.println("<TR>");
          client.println("<TH>CHANNEL</TH>");
          client.println("<TH>VALUE</TH>");
          client.println("</TR>");
          /*******************************************/
          // output the value of each analog input pin to the table
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(4); //read analogs
            /*Add the channel to the table with the class identifier*/
            client.println("<TR ALIGN=\"CENTER\">");
            client.print("<TD class=\"chan\">");
            client.print(analogChannel);
            client.println("</TD>");
            /*******************************************************/
            /*Add the coorosponding value to the table*************/
            client.print("<TD class=\"value\">");
            client.print(sensorReading);
            client.println("</TD>");
            client.println("</TR>");
            /******************************************************/
          }
          /*End the HTML****************/
          client.println("</TABLE>");
          client.println("</main>");
          client.println("</html>");
          /***************************/
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}