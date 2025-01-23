
import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
    QComboBox, QFileDialog, QLabel, QSpacerItem, QSizePolicy
)
from PyQt5.QtCore import Qt
from pyqtgraph import PlotWidget
from serial.tools import list_ports

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Seismic Data Logger")
        self.setGeometry(100, 100, 900, 600)  # Tambahkan lebar agar nyaman
        
        # Main widget
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        
        # Main layout with padding
        main_layout = QHBoxLayout()
        main_layout.setContentsMargins(10, 10, 10, 10)  # Tambahkan margin untuk jarak dengan tepi window
        main_widget.setLayout(main_layout)
        
        # Left Section (Graph Area)
        graph_layout = QVBoxLayout()
        
        # Plotting Record Graph (Lebar)
        self.graph_plotting = PlotWidget()
        self.graph_plotting.setBackground("w")
        self.graph_plotting.setTitle("Plotting Record")
        self.graph_plotting.setMinimumHeight(400)
        graph_layout.addWidget(self.graph_plotting)
        
        # Data Per Detik Graph (Lebih kecil)
        self.graph_data = PlotWidget()
        self.graph_data.setBackground("w")
        self.graph_data.setTitle("Sample Per-Second")
        self.graph_data.setMinimumHeight(100)
        graph_layout.addWidget(self.graph_data)
        
        main_layout.addLayout(graph_layout)
        
        # Add spacing between graph and controls
        main_layout.addSpacing(20)
        
        # Right Section (Controls)
        control_layout = QVBoxLayout()
        control_layout.setSpacing(10)  # Atur jarak antar elemen
        
        # Dropdown COM (Auto-detect)
        self.com_dropdown = QComboBox()
        self.refresh_com_ports()
        control_layout.addWidget(QLabel("Serial Port"))
        control_layout.addWidget(self.com_dropdown)
        
        # Save File Button
        self.save_button = QPushButton("Save")
        self.save_button.clicked.connect(self.save_file)
        control_layout.addWidget(QLabel("Save Data"))
        control_layout.addWidget(self.save_button)
        
        # Start Button
        self.start_button = QPushButton("Start")
        control_layout.addWidget(QLabel("Starting Data Logger"))
        control_layout.addWidget(self.start_button)
        
        # Stop Button
        self.stop_button = QPushButton("Stop")
        control_layout.addWidget(QLabel("Stoping Data Logger"))
        control_layout.addWidget(self.stop_button)

        # Dropdown Slave (Slave 1, 2, 3, dst.)
        self.slave_dropdown = QComboBox()
        self.slave_dropdown.addItems([f"Slave {i}" for i in range(1, 11)])  # Tambahkan 10 slave default
        control_layout.addWidget(QLabel("Choose Slave"))
        control_layout.addWidget(self.slave_dropdown)

        # Spacer
        spacer = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)
        control_layout.addItem(spacer)
        
        
        # Add control layout to main layout
        main_layout.addLayout(control_layout)
        
        # Add spacing between controls and window right edge
        main_layout.addSpacing(20)
    
    def refresh_com_ports(self):
        """Refresh COM ports and populate dropdown."""
        ports = [port.device for port in list_ports.comports()]
        if ports:
            self.com_dropdown.addItems(ports)
        else:
            self.com_dropdown.addItem("None")
    
    def save_file(self):
        file_name, _ = QFileDialog.getSaveFileName(self, "Save File", "", "CSV Files (*.csv);;All Files (*)")
        if file_name:
            print(f"File will be saved to: {file_name}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_()) 