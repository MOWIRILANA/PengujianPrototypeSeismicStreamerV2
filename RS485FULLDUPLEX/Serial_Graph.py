import sys
import serial
import threading
import re
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QWidget, QComboBox, QLabel, QTextEdit
)
from PyQt5.QtCore import QTimer, QMetaObject, Qt, Q_ARG
from pyqtgraph import PlotWidget, mkPen

# Konfigurasi Serial
SERIAL_PORT = "COM6"  # Sesuaikan dengan port serial Anda
BAUD_RATE = 115200
MAX_Y_VALUE = 3.0  # Batas atas nilai vertikal

class SerialReader(threading.Thread):
    def __init__(self, port, baudrate, callback):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.callback = callback
        self.serial_conn = None
        self.running = True

    def run(self):
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=0.1)
            print(f"Terhubung ke {self.port} dengan baudrate {self.baudrate}")
            while self.running:
                try:
                    line = self.serial_conn.readline().decode().strip()
                    if line:
                        values = self.parse_serial_data(line)
                        if values:
                            self.callback(values, line)  # Kirim data mentah juga
                except (ValueError, UnicodeDecodeError):
                    continue
        except serial.SerialException as e:
            print(f"Kesalahan Serial: {e}")

    def parse_serial_data(self, line):
        # Parsing data dengan format: "Diterima => A0: 0.6665 | A1: 0.0680 | A2: 1.0581 | Seq: 124" |\sSeq:\s(\d+
        match = re.search(
            r"A0:\s([-+]?\d*\.\d+)\s\|\sA1:\s([-+]?\d*\.\d+)\s\|\sA2:\s([-+]?\d*\.\d+)\s\|\sSeq:\s(\d+)",
            line,
        )
        if match:
            values = [float(match.group(1)), float(match.group(2)), float(match.group(3))]
            seq = int(match.group(4))
            return values, seq
        return None

    def stop(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()

class RealtimeGraph(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Real-time Serial Data Visualization")
        self.setGeometry(100, 100, 1000, 600)

        layout = QVBoxLayout()
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.central_widget.setLayout(layout)

        # Label untuk menampilkan data terbaru
        self.data_label = QLabel("Menunggu data...")
        self.data_label.setAlignment(Qt.AlignCenter)
        self.data_label.setStyleSheet("font-size: 16px; font-weight: bold;")
        layout.addWidget(self.data_label)

        # Dropdown untuk memilih data yang ingin ditampilkan
        self.data_selector = QComboBox()
        self.data_selector.addItems(["A0", "A1", "A2"])  # Sesuai data yang diterima
        self.data_selector.currentIndexChanged.connect(self.update_selected_data)
        layout.addWidget(self.data_selector)

        # Widget grafik
        self.graphWidget = PlotWidget()
        layout.addWidget(self.graphWidget)

        self.graphWidget.setBackground("w")
        self.graphWidget.setTitle("Serial Data Visualization")
        self.graphWidget.setLabel("left", "Value")
        self.graphWidget.setLabel("bottom", "Time")
        self.graphWidget.addLegend()

        self.curves = {
            "A0": self.graphWidget.plot([], [], pen=mkPen(color="r", width=2), name="A0"),
            "A1": self.graphWidget.plot([], [], pen=mkPen(color="g", width=2), name="A1"),
            "A2": self.graphWidget.plot([], [], pen=mkPen(color="b", width=2), name="A2"),
        }

        self.selected_data = "A0"  # Default pilihan A0
        self.data_x = []
        self.data_values = {"A0": [], "A1": [], "A2": []}
        self.sequence_numbers = []  # Untuk menyimpan nomor urut

        # TextEdit untuk menampilkan log data mentah
        self.raw_data_log = QTextEdit()
        self.raw_data_log.setReadOnly(True)
        self.raw_data_log.setPlaceholderText("Data mentah akan ditampilkan di sini...")
        layout.addWidget(self.raw_data_log)

        self.serial_thread = SerialReader(SERIAL_PORT, BAUD_RATE, self.store_data)
        self.serial_thread.start()

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(100)

    def store_data(self, values, raw_line):
        if values:
            values, seq = values
            index = len(self.data_x)
            self.data_x.append(index)
            self.sequence_numbers.append(seq)
            self.data_values["A0"].append(values[0])
            self.data_values["A1"].append(values[1])
            self.data_values["A2"].append(values[2])

            # Update label dengan data terbaru di thread utama
            QMetaObject.invokeMethod(
                self.data_label,
                "setText",
                Q_ARG(str, f"Data Terbaru: A0: {values[0]:.4f} | A1: {values[1]:.4f} | A2: {values[2]:.4f} | Seq: {seq}")
            )

            # Tambahkan data mentah ke log di thread utama
            QMetaObject.invokeMethod(
                self.raw_data_log,
                "append",
                Q_ARG(str, raw_line)
            )

    def update_selected_data(self):
        self.selected_data = self.data_selector.currentText()

    def update_plot(self):
        # Hanya perbarui grafik untuk data yang dipilih
        for key in self.curves:
            if key == self.selected_data:
                self.curves[key].setData(self.data_x, self.data_values[key])
            else:
                self.curves[key].setData([], [])  # Sembunyikan data lainnya

    def closeEvent(self, event):
        self.serial_thread.stop()
        self.timer.stop()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = RealtimeGraph()
    window.show()
    sys.exit(app.exec_())