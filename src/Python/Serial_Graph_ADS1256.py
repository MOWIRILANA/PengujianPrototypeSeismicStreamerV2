import sys
import serial
import threading
import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget
from PyQt5.QtCore import QTimer
import time

# Konfigurasi port serial
SERIAL_PORT = "COM16"  # Ganti dengan port yang sesuai
BAUD_RATE = 115200

# Buffer data untuk setiap kanal
data_buffer_channel_0 = []
data_buffer_channel_1 = []
data_buffer_channel_2 = []
data_buffer_channel_3 = []

# Jumlah data maksimum yang akan ditampilkan pada grafik
MAX_DATA_POINTS = 200

# Variabel untuk menghitung jumlah data yang masuk per detik
data_count_channel_0 = 0
data_count_channel_1 = 0
data_count_channel_2 = 0
data_count_channel_3 = 0
total_data_count = 0

last_update_time = time.time()


class SerialThread(threading.Thread):
    def __init__(self, serial_port, baud_rate):
        super().__init__()
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.running = True
        self.serial_conn = None

    def run(self):
        # Membuka koneksi serial
        self.serial_conn = serial.Serial(self.serial_port, self.baud_rate)
        while self.running:
            if self.serial_conn.in_waiting > 0:
                line = self.serial_conn.readline().decode('utf-8').strip()
                self.process_data(line)

    def process_data(self, line):
        global data_count_channel_0, data_count_channel_1, data_count_channel_2, data_count_channel_3, total_data_count
        try:
            # Parsing data serial
            values = line.split("||")
            if len(values) == 4:
                channel_0 = float(values[0].strip())
                channel_1 = float(values[1].strip())
                channel_2 = float(values[2].strip())
                channel_3 = float(values[3].strip())

                # Menambahkan data ke buffer masing-masing kanal
                data_buffer_channel_0.append(channel_0)
                data_buffer_channel_1.append(channel_1)
                data_buffer_channel_2.append(channel_2)
                data_buffer_channel_3.append(channel_3)

                # Menjaga ukuran buffer tetap pada batas maksimum
                if len(data_buffer_channel_0) > MAX_DATA_POINTS:
                    data_buffer_channel_0.pop(0)
                if len(data_buffer_channel_1) > MAX_DATA_POINTS:
                    data_buffer_channel_1.pop(0)
                if len(data_buffer_channel_2) > MAX_DATA_POINTS:
                    data_buffer_channel_2.pop(0)
                if len(data_buffer_channel_3) > MAX_DATA_POINTS:
                    data_buffer_channel_3.pop(0)

                # Menambah jumlah data yang diterima
                data_count_channel_0 += 1
                data_count_channel_1 += 1
                data_count_channel_2 += 1
                data_count_channel_3 += 1
                total_data_count += 1
        except Exception as e:
            print(f"Error processing data: {e}")

    def stop(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Real-time Serial Data')
        self.setGeometry(100, 100, 800, 600)

        # Layout utama
        layout = QVBoxLayout()

        # Inisialisasi plot PyQtGraph
        self.plot_widget = pg.PlotWidget(self)
        self.plot_widget.setBackground('w')
        layout.addWidget(self.plot_widget)

        # Menambahkan kurva plot untuk setiap kanal
        self.curve_channel_0 = self.plot_widget.plot(pen=pg.mkPen(color='r', width=2), name="Channel 0 (Red)")
        self.curve_channel_1 = self.plot_widget.plot(pen=pg.mkPen(color='g', width=2), name="Channel 1 (Green)")
        self.curve_channel_2 = self.plot_widget.plot(pen=pg.mkPen(color='b', width=2), name="Channel 2 (Blue)")
        self.curve_channel_3 = self.plot_widget.plot(pen=pg.mkPen(color='y', width=2), name="Channel 3 (Yellow)")

        # Label untuk statistik
        self.stats_label = QLabel("Data Statistics:", self)
        layout.addWidget(self.stats_label)

        # Membuat widget untuk jendela utama
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        # Timer untuk memperbarui plot dan statistik
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(50)  # Memperbarui setiap 50 ms

        # Memulai thread serial
        self.serial_thread = SerialThread(SERIAL_PORT, BAUD_RATE)
        self.serial_thread.start()

    def update_plot(self):
        global data_count_channel_0, data_count_channel_1, data_count_channel_2, data_count_channel_3, total_data_count, last_update_time

        # Memperbarui data pada plot
        self.curve_channel_0.setData(data_buffer_channel_0)
        self.curve_channel_1.setData(data_buffer_channel_1)
        self.curve_channel_2.setData(data_buffer_channel_2)
        self.curve_channel_3.setData(data_buffer_channel_3)

        # Hitung data rate per detik
        current_time = time.time()
        time_diff = current_time - last_update_time
        if time_diff >= 1:
            rate_channel_0 = data_count_channel_0 / time_diff
            rate_channel_1 = data_count_channel_1 / time_diff
            rate_channel_2 = data_count_channel_2 / time_diff
            rate_channel_3 = data_count_channel_3 / time_diff
            total_rate = total_data_count / time_diff

            # Tampilkan statistik
            self.stats_label.setText(
                f"Data Rate:\n"
                f"  Reg 0 (Red): {rate_channel_0:.2f} data/sec\n"
                f"  Reg 1 (Green): {rate_channel_1:.2f} data/sec\n"
                f"  Reg 2 (Blue): {rate_channel_2:.2f} data/sec\n"
                f"  Reg 3 (Yellow): {rate_channel_3:.2f} data/sec\n"
                f"  Average Rate: {total_rate:.2f} data/sec"
            )

            # Reset penghitung data
            data_count_channel_0 = 0
            data_count_channel_1 = 0
            data_count_channel_2 = 0
            data_count_channel_3 = 0
            total_data_count = 0
            last_update_time = current_time

    def closeEvent(self, event):
        # Menghentikan thread serial saat jendela ditutup
        self.serial_thread.stop()
        self.serial_thread.join()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
