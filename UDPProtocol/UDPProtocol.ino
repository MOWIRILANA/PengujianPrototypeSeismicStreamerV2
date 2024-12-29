#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "local_config.h"	

const int UDP_PACKET_SIZE = 24;		// UDP time stamp is in the first 48 bytes of the message.
char packetBuffer[UDP_PACKET_SIZE];	// Buffer for both incoming and outgoing packets.
char ReplyBuffer[] = "acknowledged";

EthernetUDP Udp;

const char* message = "Hello, this is ESP32 sending data!";
// IPAddress remoteIP(169,254,98,18);  // Replace with the IP of the laptop or server
// const unsigned int remotePort = 8888; // Port to send data to

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

void prt_hwval(uint8_t refval) {
    switch (refval) {
    case 0:
        Serial.println("No hardware detected.");
        break;
    case 1:
        Serial.println("WizNet W5100 detected.");
        break;
    case 2:
        Serial.println("WizNet W5200 detected.");
        break;
    case 3:
        Serial.println("WizNet W5500 detected.");
        break;
    default:
        Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
    }
}

void prt_ethval(uint8_t refval) {
    switch (refval) {
    case 0:
        Serial.println("Unknown status.");
        break;
    case 1:
        Serial.println("Link flagged as UP.");
        break;
    case 2:
        Serial.println("Link flagged as DOWN. Check cable connection.");
        break;
    default:
        Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
    }
}

void cableConnection(){
      // Check W5500 and cable connection
    Serial.print("Checking connection.");
    bool rdy_flag = false;
    for (uint8_t i = 0; i <= 20; i++) {
        if ((Ethernet.hardwareStatus() == EthernetNoHardware) || (Ethernet.linkStatus() == LinkOFF)) {
            Serial.print(".");
            rdy_flag = false;
            delay(80);
        } else {
            rdy_flag = true;
            break;
        }
    }
    if (rdy_flag == false) {
        Serial.println("\n\r\tHardware fault, or cable problem... cannot continue.");
        Serial.print("Hardware Status: ");
        prt_hwval(Ethernet.hardwareStatus());
        Serial.print("   Cable Status: ");
        prt_ethval(Ethernet.linkStatus());
        while (true) {
            delay(10);
        }
    } else {
        Serial.println(" OK");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\tUDP Client v1.0\r\n");

    Ethernet.init(15);
    WizReset();

    // Network configuration
    Serial.println("Starting ETHERNET connection...");
    Ethernet.begin(eth_MAC, eth_IP, eth_DNS, eth_GW, eth_MASK);

    delay(200);
    // Info Ethernet
    Serial.print("Ethernet IP is: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask : ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP : ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server : ");
    Serial.println(Ethernet.dnsServerIP());

    cableConnection();
    Udp.begin(localPort); // Start UDP communication
}

void loop() {
    
    // Send a UDP packet
    Serial.println("Sending data via UDP...");
    if (Udp.beginPacket(eth_IP, localPort)) {
    Udp.write(message);
    if (Udp.endPacket()) {
        Serial.println("UDP packet sent successfully!");
    } else {
        Serial.println("Error sending UDP packet.");
    }
  }   
    else {
        Serial.println("Failed to start UDP packet.");
    }

    delay(1000);  // Delay for 1 second before sending the next message
}
