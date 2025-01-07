uint8_t eth_MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };


IPAddress eth_IP(10, 14, 1, 21);		// *** CHANGE THIS to something relevant for YOUR LAN. ***
IPAddress eth_MASK(255, 255, 0, 0);		// Subnet mask.
IPAddress eth_DNS(10, 14, 0, 1);		// *** CHANGE THIS to match YOUR DNS server.           ***
IPAddress eth_GW(10, 14, 0, 1);		// *** CHANGE THIS to match YOUR Gateway (router).     ***

#define RESET_P	27				// Tie the Wiz820io/W5500 reset pin to ESP32 GPIO26 pin.

const uint16_t localPort = 5000;		// Local port for UDP packets.