uint8_t eth_MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };


IPAddress eth_IP(192, 168, 43, 157);		// *** CHANGE THIS to something relevant for YOUR LAN. ***
IPAddress eth_MASK(255, 255, 255, 0);		// Subnet mask.
IPAddress eth_DNS(192, 168, 43, 1);		// *** CHANGE THIS to match YOUR DNS server.           ***
IPAddress eth_GW(192, 168, 43, 1);		// *** CHANGE THIS to match YOUR Gateway (router).     ***

#define RESET_P	4				// Tie the Wiz820io/W5500 reset pin to ESP32 GPIO26 pin.

const uint16_t localPort = 5000;		// Local port for UDP packets.