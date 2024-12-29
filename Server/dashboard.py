import socket

# Konfigurasi server
host = "0.0.0.0"  # Mendengarkan di semua antarmuka jaringan
port = 58642      # Port server

# Membuat socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((host, port))  # Bind ke alamat dan port

print(f"Server listening on {host}:{port}...")

# Menunggu data
while True:
    data, client_address = server_socket.recvfrom(1024)  # Menerima data
    print(f"Received: {data.decode()} from {client_address}")

    # Mengirim balasan
    server_socket.sendto("Hello from server!".encode(), client_address)
