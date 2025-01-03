import socket

# Konfigurasi server
UDP_IP = "0.0.0.0"  # Mendengarkan di semua antarmuka
UDP_PORT = 5000     # Port yang digunakan (sesuai dengan pengaturan ESP32)

# Membuat socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)  # Izinkan menerima broadcast
sock.bind((UDP_IP, UDP_PORT))

print(f"Server listening on {UDP_IP}:{UDP_PORT}")

while True:
    try:
        data, addr = sock.recvfrom(1024)  # Buffer size 1024 bytes
        print(f"Received message: {data.decode()} from {addr}")
    except KeyboardInterrupt:
        print("\nServer shutting down.")
        break
    except Exception as e:
        print(f"Error: {e}")