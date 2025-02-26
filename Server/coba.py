import spidev
import time

# Initialize SPIs
spi = spidev.SpiDev()
spi.open(0, 0)  # Open bus 0, device 0
spi.max_speed_hz = 50000

def spi_transfer(data):
    response = spi.xfer2(data)
    return response

try:
    while True:
        send_data = [0x01, 0x02, 0x03]  # Example data to send
        received_data = spi_transfer(send_data)
        print(f"Sent: {send_data}, Received: {received_data}")
        time.sleep(1)
except KeyboardInterrupt:
    spi.close()