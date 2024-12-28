#define ETHERNET_MAC            "A0:1D:48:F3:86:CF" // Ethernet MAC address (have to be unique between devices in the same network)
#define ETHERNET_IP             "255.255.255.255"       // IP address of RoomHub when on Ethernet connection

#define ETHERNET_RESET_PIN      26      // ESP32 pin where reset pin from W5500 is connected
#define ETHERNET_CS_PIN         15       // ESP32 pin where CS pin from W5500 is connected

#define TCP_HOSTNAME           "10.0.88.87"
#define TCP_PORT               6789

#define TCP_PUBLISH_INTERVAL_MS   1000