import sys
import numpy as np
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
import pyqtgraph as pg

class SeismicViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Seismic Data Viewer")
        self.setGeometry(100, 100, 800, 600)
        
        # Membuat widget utama dan layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Membuat plot widget
        plot_widget = pg.PlotWidget()
        layout.addWidget(plot_widget)
        
        # Konfigurasi plot
        plot_widget.setBackground('w')  # Background putih
        plot_widget.showGrid(x=True, y=True, alpha=0.5)
        plot_widget.setLabel('left', 'Time (ms)')
        plot_widget.setLabel('bottom', 'Trace Number (Stacked)')
        plot_widget.setTitle('Seismic Plot - Synthetic Data', size='14pt')
        
        # Generate dan plot data
        self.plot_seismic_data(plot_widget)
        
    def create_synthetic_seismic_data(self, num_traces=50, num_samples=500, noise_level=0.1):
        """
        Membuat data sintetik seismic trace.
        """
        np.random.seed(42)
        traces = np.zeros((num_samples, num_traces))
        time = np.linspace(0, 1, num_samples)
        freq = 20
        
        for i in range(num_traces):
            wavelet = np.sin(2 * np.pi * freq * time) * np.exp(-20 * (time - 0.5)**2)
            traces[:, i] = wavelet + noise_level * np.random.randn(num_samples)
            
        return traces
    
    def plot_seismic_data(self, plot_widget):
        # Generate data
        num_traces = 50
        num_samples = 500
        seismic_data = self.create_synthetic_seismic_data(num_traces, num_samples)
        
        # Plot setiap trace
        offset = 0.5
        for i in range(num_traces):
            trace = seismic_data[:, i]
            plot_widget.plot(trace + i * offset, 
                           np.arange(num_samples),
                           pen=pg.mkPen('k', width=0.8))  # Warna hitam
        
        # Membalikkan sumbu Y
        plot_widget.getViewBox().invertY(True)

def main():
    app = QApplication(sys.argv)
    viewer = SeismicViewer()
    viewer.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()