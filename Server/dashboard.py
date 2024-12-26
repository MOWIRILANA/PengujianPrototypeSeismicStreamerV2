import socket

# Define the UDP listening IP address and port
localIP = "0.0.0.0"  # Listen on all available network interfaces
localPort = 12345     # Same port as the ESP32 is sending to

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((localIP, localPort))  # Bind the socket to the IP and port

print(f"Listening for UDP packets on port {localPort}...")

# Receive UDP packets in a loop
while True:
    data, addr = sock.recvfrom(1024)  # Buffer size is 1024 bytes
    print(f"Received message: {data.decode()} from {addr}")
