/*
 *   local_config.h for ESP32 2 (Slave)
 */

/*
 * W5500 "hardware" MAC address.
 */
uint8_t eth_MAC[] = { 0xA0, 0x1D, 0x48, 0xF2, 0x86, 0xCE };

/*
 * Define the static network settings for this gateway's ETHERNET connection
 * on your LAN. These values must match YOUR SPECIFIC LAN.
 */
IPAddress eth_IP(192, 168, 47, 101);  // Unique IP address for ESP32 2
IPAddress eth_MASK(255, 255, 255, 0); // Subnet mask
IPAddress eth_DNS(192, 168, 47, 229); // DNS Server
IPAddress eth_GW(192, 168, 47, 229);  // Gateway

#define RESET_P 26                // Tie the Wiz820io/W5500 reset pin to ESP32 GPIO26 pin.

const uint16_t localPort = 5000;  // Local port for UDP packets

/*
 * NTP Server Configuration
 */
const char timeServer[] = "asia.pool.ntp.org";

const uint8_t SLEEP_SECS = 15;    // Number of seconds to sleep between queries
