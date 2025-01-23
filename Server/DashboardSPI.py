import serial
import threading
import matplotlib.pyplot as plt
from collections import deque
import time

# Konfigurasi port serial
serial_port = 'COM3'  # Ganti sesuai dengan port serial Anda
baud_rate = 57600

# Variabel untuk menyimpan data
adc_values = deque(maxlen=10000)  # Menyimpan maksimal 100 data
time_stamps = deque(maxlen=10000)
lock = threading.Lock()  # Untuk sinkronisasi antara thread

# Fungsi untuk membaca data dari serial
def read_serial():
    global adc_values, time_stamps
    try:
        ser = serial.Serial(serial_port, baud_rate)
        while True:
            line = ser.readline().decode('utf-8').strip()  # Baca satu baris data
            if line.startswith("ADC"):  # Filter data ADC
                _, value = line.split(",")  # Pisahkan label dan nilai
                with lock:  # Sinkronisasi akses ke data
                    adc_values.append(int(value))
                    time_stamps.append(time.time())  # Gunakan waktu dalam detik
    except Exception as e:
        print(f"Error in serial thread: {e}")

# Fungsi untuk memperbarui grafik
def update_graph():
    plt.ion()  # Aktifkan mode interaktif
    fig, ax = plt.subplots()
    while True:
        with lock:  # Sinkronisasi akses ke data
            if len(adc_values) > 0:
                ax.clear()
                times = [t - time_stamps[0] for t in time_stamps]  # Waktu relatif
                ax.plot(times, adc_values, label="ADC Value")
                ax.set_title("Real-time ADC Data")
                ax.set_xlabel("Time (seconds)")
                ax.set_ylabel("ADC Value")
                ax.legend()
                plt.pause(0.1)  # Perbarui grafik setiap 100 ms

# Membuat dan menjalankan thread
serial_thread = threading.Thread(target=read_serial, daemon=True)
graph_thread = threading.Thread(target=update_graph, daemon=True)

serial_thread.start()
graph_thread.start()

# Menjaga program tetap berjalan
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Program dihentikan.")
