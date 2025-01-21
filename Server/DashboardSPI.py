import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from threading import Thread, Lock
import time

# Pastikan port serial sesuai dengan perangkat Anda
ser = serial.Serial('COM4', 115200, timeout=1)

# Data untuk menyimpan pembacaan
x_data = []
y1_data = []
y2_data = []
y3_data = []

# Kunci untuk sinkronisasi akses data
data_lock = Lock()

# Penghitung jumlah data per detik
data_count = 0
data_sum = 0
last_time = time.time()

def read_serial():
    global data_count, data_sum
    frame = 0
    while True:
        line = ser.readline().decode('utf-8').strip()
        try:
            v0, v1, v2 = map(float, line.split(','))
            with data_lock:  # Sinkronisasi akses data
                x_data.append(frame)
                y1_data.append(v0)
                y2_data.append(v1)
                y3_data.append(v2)

                data_count += 1  # Hitung data yang diterima
                data_sum += (v0 + v1 + v2)  # Tambahkan nilai untuk menghitung rata-rata

                if len(x_data) > 200:  # Batasi panjang data
                    x_data.pop(0)
                    y1_data.pop(0)
                    y2_data.pop(0)
                    y3_data.pop(0)
            frame += 1
        except ValueError:
            pass  # Abaikan baris yang tidak dapat diproses

def init():
    ax.set_xlim(0, 200)
    ax.set_ylim(-1000, 1000)  # Sesuaikan range sesuai kebutuhan
    ax.set_xlabel('Sample')
    ax.set_ylabel('Geophone Signal')
    ax.legend()
    return line1, line2, line3, count_text, avg_text

def update(frame):
    global data_count, data_sum, last_time
    current_time = time.time()

    # Hitung data per detik
    if current_time - last_time >= 1:
        last_time = current_time
        with data_lock:  # Sinkronisasi akses data
            count_text.set_text(f'Data/s: {data_count}')
            if data_count > 0:
                avg_data = data_sum / (data_count * 3)  # Rata-rata dari 3 sinyal
                avg_text.set_text(f'Avg Data/s: {avg_data:.2f}')
            else:
                avg_text.set_text(f'Avg Data/s: 0.00')
        data_count = 0  # Reset penghitung data
        data_sum = 0  # Reset jumlah data

    with data_lock:  # Sinkronisasi akses data
        line1.set_data(range(len(x_data)), y1_data)
        line2.set_data(range(len(x_data)), y2_data)
        line3.set_data(range(len(x_data)), y3_data)
    return line1, line2, line3, count_text, avg_text

# Setup Matplotlib
fig, ax = plt.subplots()
line1, = ax.plot([], [], label='Geophone X')
line2, = ax.plot([], [], label='Geophone Y')
line3, = ax.plot([], [], label='Geophone Z')
count_text = ax.text(0.95, 0.95, '', transform=ax.transAxes, ha='right', va='top')
avg_text = ax.text(0.95, 0.90, '', transform=ax.transAxes, ha='right', va='top')

# Jalankan thread untuk membaca data serial
serial_thread = Thread(target=read_serial, daemon=True)
serial_thread.start()

# Animasi grafik
ani = FuncAnimation(fig, update, init_func=init, interval=50, blit=True)
plt.show()
