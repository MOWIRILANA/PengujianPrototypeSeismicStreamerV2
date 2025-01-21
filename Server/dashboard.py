import socket
import time

# Konfigurasi server
UDP_IP = "0.0.0.0"  # Mendengarkan di semua antarmuka
UDP_PORT = 5000     # Port yang digunakan (sesuai dengan pengaturan ESP32)

# Membuat socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)  # Izinkan menerima broadcast
sock.bind((UDP_IP, UDP_PORT))

print(f"Server listening on {UDP_IP}:{UDP_PORT}")

packet_count = 0
last_time = time.time()

while True:
    try:
        data, addr = sock.recvfrom(1024)  # Buffer size 1024 bytes
        packet_count += 1
        print(f"Received message: {data.decode()} from {addr}")
        
        current_time = time.time()
        if current_time - last_time >= 1:  # Hitung per detik
            print(f"Packets received in the last second: {packet_count}")
            packet_count = 0
            last_time = current_time
    except KeyboardInterrupt:
        print("\nServer shutting down.")
        break
    except Exception as e:
        print(f"Error: {e}")
