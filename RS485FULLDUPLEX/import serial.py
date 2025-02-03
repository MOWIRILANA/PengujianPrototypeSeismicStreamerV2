import serial
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Konfigurasi port USB RS485 (Gantilah sesuai sistem Anda)
PORT = "COM9"  # Untuk Linux/Mac (contoh: "/dev/ttyUSB0", "/dev/ttyS0")
# PORT = "COM3"  # Untuk Windows (contoh: "COM3", "COM4")
BAUDRATE = 115200

# Buffer untuk menyimpan data terbaru
data_buffer = deque(maxlen=50)

# Fungsi untuk membaca data dari RS485
def read_rs485():
    with serial.Serial(PORT, BAUDRATE, timeout=1) as ser:
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(f"Received: {line}")
                    data_buffer.append(float(line))  # Menyimpan data dalam buffer
            except Exception as e:
                print(f"Error: {e}")

# Fungsi untuk memperbarui grafik real-time
def update_graph(frame):
    ax.clear()
    ax.plot(list(data_buffer), marker='o', linestyle='-')
    ax.set_title("Real-time RS485 Data")
    ax.set_xlabel("Sample Index")
    ax.set_ylabel("Voltage (V)")
    ax.grid(True)

# Memulai thread untuk membaca data dari RS485
thread = threading.Thread(target=read_rs485, daemon=True)
thread.start()

# Setup Matplotlib
fig, ax = plt.subplots()
ani = animation.FuncAnimation(fig, update_graph, interval=500)

plt.show()
