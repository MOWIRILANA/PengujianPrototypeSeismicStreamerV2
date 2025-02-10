import sys
import serial
import threading
import time
from matplotlib.animation import FuncAnimation
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QComboBox, QLineEdit, QFormLayout
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import matplotlib.pyplot as plt
import numpy as np
from scipy.signal import spectrogram


class GUILogger(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()
        self.serial_port = None
        self.baudrate = None
        self.data = []
        self.sample_rate = 0
        self.start_time = time.time()
        self.samples_per_second = []
        self.running = True

    def initUI(self):
        self.setWindowTitle('Seismic Streamer GUI Logger')
        self.setGeometry(100, 100, 800, 600)

        main_layout = QVBoxLayout()
        top_layout = QHBoxLayout()
        graph_layout = QVBoxLayout()
        control_layout = QVBoxLayout()
        status_layout = QVBoxLayout()
        form_layout = QFormLayout()
        bottom_graph_layout = QVBoxLayout()

        # Graph area (top graph)
        self.figure, self.ax = plt.subplots()
        self.canvas = FigureCanvas(self.figure)
        graph_layout.addWidget(self.canvas)

        # Additional bottom graph
        self.figure2, self.ax2 = plt.subplots()
        self.canvas2 = FigureCanvas(self.figure2)
        bottom_graph_layout.addWidget(self.canvas2)

        # Control area
        self.port_label = QLabel('Serial Port:')
        self.port_combo = QComboBox()
        self.port_combo.addItems(self.get_serial_ports())
        self.baudrate_label = QLabel('Baudrate:')
        self.baudrate_combo = QComboBox()
        self.baudrate_combo.addItems(self.get_baud_rate())
        self.show_spectrograms = QPushButton('Show Spectrogram')
        self.show_spectrograms.clicked.connect(self.show_spectrogram)
        self.connect_button = QPushButton('Connect')
        self.connect_button.clicked.connect(self.connect_serial)

        form_layout.addRow(self.port_label, self.port_combo)
        form_layout.addRow(self.baudrate_label, self.baudrate_combo)
        control_layout.addLayout(form_layout)
        control_layout.addWidget(self.show_spectrograms)
        control_layout.addWidget(self.connect_button)

        # Status area
        self.sample_rate_label = QLabel('Sample Rate: 0 SPS')
        status_layout.addWidget(self.sample_rate_label)

        control_layout.addLayout(status_layout)
        
        # Combine layouts
        top_layout.addLayout(graph_layout, 3)  # Lebih besar untuk grafik
        top_layout.addLayout(control_layout, 1)  # Kontrol di sebelah kanan
        
        main_layout.addLayout(top_layout)
        main_layout.addLayout(bottom_graph_layout)  # Tambahkan grafik bawah
        
        self.setLayout(main_layout)

        # Start the animation
        self.ani = FuncAnimation(self.figure, self.update_graph, interval=4095)
        self.ani2 = FuncAnimation(self.figure2, self.update_graph2, interval=4095)

    def get_baud_rate(self):
        return ['9600', '19200', '38400','57600', '115200']
    
    def get_serial_ports(self):
        return ['COM1', 'COM2', 'COM3', 'COM4']

    def connect_serial(self):
        port = self.port_combo.currentText()
        baudrate = self.baudrate_combo.currentText()
        self.serial_port = serial.Serial(port, baudrate)
        self.serial_thread = threading.Thread(target=self.read_serial_data)
        self.serial_thread.start()

    def show_spectrogram(self):
        #read analog data from esp32 analog pin
        analog_signal = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

        # Generate a sample analog signal (sine wave)
        fs = 1000
        t = np.linspace(0, 1, fs, endpoint=False)
        f = 5
        analog_signal = np.sin(2 * np.pi * f * t)
        
        # Compute the spectrogram
        frequencies, times, Sxx = spectrogram(analog_signal, fs)
        
        # Plot the spectrogram
        plt.pcolormesh(times, frequencies, 10 * np.log10(Sxx), shading='gouraud')
        plt.ylabel('Frequency [Hz]')
        plt.xlabel('Time [sec]')
        plt.title('Spectrogram')
        plt.colorbar(label='Intensity [dB]')
        plt.show()        

    def read_serial_data(self):
        while self.running:
            if self.serial_port and self.serial_port.in_waiting:
                data = self.serial_port.readline().decode('utf-8').strip()
            
                # Mencari pola data yang ada di format "A0: xxx | A1: xxx | A2: xxx | A3: xxx"
                analog_values = {}
                parts = data.split(' | ')  # Memisahkan berdasarkan '|'
                
                for part in parts:
                    label, value = part.split(': ')  # Memisahkan label dan nilai
                    analog_values[label] = int(value)  # Menyimpan nilai analog sebagai integer
                
                # Pastikan data yang diterima berisi 4 saluran
                if len(analog_values) == 4:
                    # Menambahkan data analog ke dalam list data
                    self.data.append([analog_values['A0'], analog_values['A1'], analog_values['A2'], analog_values['A3']])

                    # Menghitung sample rate
                    self.sample_rate = len(self.data) / (time.time() - self.start_time)
                    self.sample_rate_label.setText(f'Sample Rate: {self.sample_rate:.2f} SPS')
                    self.samples_per_second.append(self.sample_rate)

    def update_graph(self, frame):
        # Menyiapkan data untuk grafik vertikal
        a0_data = [d[0] for d in self.data]
        a1_data = [d[1] for d in self.data]
        a2_data = [d[2] for d in self.data]
        a3_data = [d[3] for d in self.data]

        # Menghapus sumbu dan menggambar data vertikal
        self.ax.clear()

        # Plot data analog sebagai garis vertikal
        self.ax.plot(a0_data, label="A0")
        self.ax.plot(a1_data, label="A1")
        self.ax.plot(a2_data, label="A2")
        self.ax.plot(a3_data, label="A3")

        # Menambahkan label
        self.ax.set_title("Data Analog")
        self.ax.set_xlabel("Sampel")
        self.ax.set_ylabel("Nilai Analog")
        self.ax.legend()

        # tampilkan spektrogram
        # self.ax.show()


        # self.ax.show()
        # Redraw canvas
        # plt.show()
        self.canvas.draw()

    def update_graph2(self, frame):
        # Plot sample rate per detik
        self.ax2.clear()
        self.ax2.plot(self.samples_per_second)
        self.ax2.set_title("Sample Rate per Second")
        self.canvas2.draw()
    
    def closeEvent(self, event):
        self.running = False
        if self.serial_port:
            self.serial_port.close()
        event.accept()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    gui_logger = GUILogger()
    gui_logger.showMaximized()
    sys.exit(app.exec_())
