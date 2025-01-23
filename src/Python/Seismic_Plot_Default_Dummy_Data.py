import sys
import numpy as np
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt5.QtCore import QTimer
import pyqtgraph as pg

class RealtimeSeismicViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Realtime Seismic Data Viewer")
        self.setGeometry(100, 100, 800, 600)
        
        # Parameter data
        self.num_traces = 200  # Batas awal
        self.num_samples = 500
        self.current_time = 0
        
        # Inisialisasi buffer data
        self.data_buffer = np.zeros((self.num_samples, self.num_traces))
        
        # Setup UI
        self.setup_ui()
        
        # Setup timer untuk update realtime
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(1000)  # Update setiap 1000ms (1 detik)
        
    def setup_ui(self):
        # Membuat widget utama dan layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Membuat plot widget
        self.plot_widget = pg.PlotWidget()
        layout.addWidget(self.plot_widget)
        
        # Konfigurasi plot
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.5)
        self.plot_widget.setLabel('left', 'Time (ms)')
        self.plot_widget.setLabel('bottom', 'Trace Number (Stacked)')
        self.plot_widget.setTitle('Realtime Seismic Plot', size='14pt')
        
        # Membalikkan sumbu Y
        self.plot_widget.getViewBox().invertY(True)
        
        # Inisialisasi plot lines
        self.plot_lines = []
        self.max_traces = self.num_traces  # Menyimpan jumlah trace yang saat ini digunakan
        self.add_plot_lines(self.num_traces)  # Menambahkan garis plot sesuai jumlah trace awal
    
    def add_plot_lines(self, num_traces):
        """Menambahkan garis plot sesuai dengan jumlah trace"""
        offset = 0.5
        for i in range(num_traces):
            line = self.plot_widget.plot(pen=pg.mkPen('k', width=0.8))
            self.plot_lines.append(line)

    def generate_new_trace(self):
        """
        Menghasilkan satu trace data baru
        """
        time = np.linspace(0, 1, self.num_samples)
        freq = 20
        phase = self.current_time * 2 * np.pi  # Mengubah fase untuk animasi
        
        # Membuat gelombang bergerak
        wavelet = np.sin(2 * np.pi * freq * time + phase) * np.exp(-20 * (time - 0.5)**2)
        noise = 0.1 * np.random.randn(self.num_samples)
        
        return wavelet + noise
    
    def update_data(self):
        # Geser data lama ke kiri
        self.data_buffer = np.roll(self.data_buffer, -1, axis=1)
        
        # Generate trace baru
        new_trace = self.generate_new_trace()
        self.data_buffer[:, -1] = new_trace
        
        # Cek apakah jumlah trace melebihi batas
        if self.num_traces > self.max_traces:
            self.add_plot_lines(self.num_traces - self.max_traces)  # Menambahkan lebih banyak garis plot
            self.max_traces = self.num_traces  # Memperbarui batas maksimal trace
        
        # Update plot
        offset = 1
        for i in range(self.num_traces):
            trace = self.data_buffer[:, i]
            self.plot_lines[i].setData(trace + i * offset, np.arange(self.num_samples))
        
        self.current_time += 1

def main():
    app = QApplication(sys.argv)
    viewer = RealtimeSeismicViewer()
    viewer.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
