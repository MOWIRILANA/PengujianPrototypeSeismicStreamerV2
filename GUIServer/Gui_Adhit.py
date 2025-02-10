import asyncio
from pymodbus.client import AsyncModbusSerialClient
from pymodbus.exceptions import ModbusException, ConnectionException
from pymodbus.pdu import ExceptionResponse
import logging
import sys
from datetime import datetime
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets, QtCore
import threading
import time
import numpy as np

# Enable anti-aliasing for smoother plots
pg.setConfigOptions(antialias=True)

# Logging configuration
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("modbus_accumulated_plot.log"),
    ],
)

# Global variables
lock = threading.Lock()
data_buffer = {}
MAX_ITERATIONS = 500  # Maximum number of data points to accumulate
data_running = False  # Global variable to control data transmission

class ModbusRTUMaster:
    def __init__(self, port, slave_ids, baudrate=9600, timeout=3, max_retries=5, retry_delay=1):
        self.port = port
        self.slave_ids = slave_ids
        self.baudrate = baudrate
        self.timeout = timeout
        self.client = None
        self.connected = False
        self.max_retries = max_retries
        self.retry_delay = retry_delay

    async def connect(self):
        retries = 0
        while retries < self.max_retries:
            try:
                if self.client and self.client.connected:
                    await self.client.close()  # Ensure the port is closed before reconnecting
                self.client = AsyncModbusSerialClient(
                    port=self.port,
                    baudrate=self.baudrate,
                    bytesize=8,
                    parity="N",
                    stopbits=1,
                    timeout=self.timeout,
                )
                self.connected = await self.client.connect()
                if self.connected:
                    logging.info(f"Connected to {self.port} at {self.baudrate} baud")
                    return True
                else:
                    retries += 1
                    await asyncio.sleep(self.retry_delay)
            except Exception as e:
                retries += 1
                logging.warning(f"Connection error: {e}. Retry {retries}/{self.max_retries}")
                await asyncio.sleep(self.retry_delay)
        logging.error(f"Connection failed after {self.max_retries} retries.")
        return False

    async def disconnect(self):
        if self.client and self.client.connected:
            await self.client.close()
            self.connected = False
            logging.info("Disconnected")

    async def read_voltages(self, slave_id):
        try:
            if not self.connected:
                await self.connect()
            response = await self.client.read_holding_registers(address=0, count=4, slave=slave_id)
            if isinstance(response, ExceptionResponse):
                raise ModbusException(f"Slave {slave_id} exception")
            if hasattr(response, "registers"):
                return [value / 1000.0 for value in response.registers]
            else:
                raise ModbusException("Invalid response")
        except (ConnectionException, ModbusException) as e:
            logging.error(f"Read error slave {slave_id}: {e}")
            self.connected = False
        except Exception as e:
            logging.error(f"Unexpected error: {e}")
        return None


async def modbus_main(master, shutdown_event):
    global data_buffer, data_running
    try:
        iteration = 0
        while not shutdown_event.is_set():
            if data_running:
                for slave_id in master.slave_ids:
                    voltages = await master.read_voltages(slave_id)
                    if voltages:
                        timestamp = datetime.now().timestamp()
                        with lock:
                            if slave_id not in data_buffer:
                                data_buffer[slave_id] = {}
                            for i, reg in enumerate(["A0", "A1", "A2", "A3"]):
                                if reg not in data_buffer[slave_id]:
                                    data_buffer[slave_id][reg] = []
                                data_buffer[slave_id][reg].append((iteration, voltages[i], timestamp))
                                # Limit data to MAX_ITERATIONS
                                if len(data_buffer[slave_id][reg]) > MAX_ITERATIONS:
                                    data_buffer[slave_id][reg].pop(0)
                iteration += 1
            await asyncio.sleep(0.0001)  # Faster polling
    except Exception as e:
        logging.error(f"Modbus loop error: {e}")
    finally:
        await master.disconnect()


class AccumulatedPlotViewer(QtWidgets.QMainWindow):
    def __init__(self, slave_ids):
        super().__init__()
        self.slave_ids = slave_ids
        self.init_ui()
        self.init_data()
        self.start_timers()

    def init_ui(self):
        self.setWindowTitle("Accumulated Line Plot Viewer - Modbus")
        self.setGeometry(100, 100, 1000, 600)

        main_widget = QtWidgets.QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QtWidgets.QHBoxLayout(main_widget)  # Menggunakan QHBoxLayout sebagai layout utama

        # Plot widget
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.plot_widget.setLabel('left', 'Voltage (V)')
        self.plot_widget.setLabel('bottom', 'Iteration')

        # Control panel
        control_panel = QtWidgets.QWidget()
        control_layout = QtWidgets.QVBoxLayout(control_panel)  # Menggunakan QVBoxLayout untuk panel kontrol

        # Dropdown untuk memilih slave dan register
        self.slave_dropdown = QtWidgets.QComboBox()
        self.slave_dropdown.addItems([f"Slave {sid}" for sid in self.slave_ids])
        self.register_dropdown = QtWidgets.QComboBox()
        self.register_dropdown.addItems(["A0", "A1", "A2", "A3"])

        # ComboBox untuk memilih baudrate
        self.baudrate_dropdown = QtWidgets.QComboBox()
        self.baudrate_dropdown.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.baudrate_dropdown.setCurrentText("115200")  # Set default baudrate

        # Label untuk menampilkan sample rate dan total data rate
        self.sample_rate_label = QtWidgets.QLabel("Sample Rate: N/A Hz")
        self.total_data_rate_label = QtWidgets.QLabel("Total Data Rate: N/A Hz")
        self.sample_rate_label.setStyleSheet("font-weight: bold; color: blue;")
        self.total_data_rate_label.setStyleSheet("font-weight: bold; color: green;")

        # Tombol Start dan Stop
        self.start_button = QtWidgets.QPushButton("Start")
        self.stop_button = QtWidgets.QPushButton("Stop")
        self.start_button.clicked.connect(self.start_data)
        self.stop_button.clicked.connect(self.stop_data)

        # Tambahkan widget ke control_layout
        control_layout.addWidget(QtWidgets.QLabel("Slave:"))
        control_layout.addWidget(self.slave_dropdown)
        control_layout.addWidget(QtWidgets.QLabel("Register:"))
        control_layout.addWidget(self.register_dropdown)
        control_layout.addWidget(QtWidgets.QLabel("Baudrate:"))
        control_layout.addWidget(self.baudrate_dropdown)
        control_layout.addWidget(self.sample_rate_label)
        control_layout.addWidget(self.total_data_rate_label)
        control_layout.addWidget(self.start_button)
        control_layout.addWidget(self.stop_button)
        control_layout.addStretch()  # Menambahkan stretch untuk mengisi ruang kosong

        # Tambahkan plot_widget dan control_panel ke main_layout
        main_layout.addWidget(self.plot_widget, stretch=4)  # Grafik mengambil 4 bagian dari ruang
        main_layout.addWidget(control_panel, stretch=1)  # Panel kontrol mengambil 1 bagian dari ruang

    def init_data(self):
        self.current_trace = None

    def start_timers(self):
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(50)  # Update plot every 200 ms

    def downsample_data(self, x, y, max_points=500):
        """Downsample data to reduce the number of points while preserving the shape."""
        if len(x) > max_points:
            indices = np.linspace(0, len(x) - 1, max_points).astype(int)
            return x[indices], y[indices]
        return x, y

    def process_data(self):
        global data_buffer
        processed = []

        with lock:
            selected_slave = self.slave_ids[self.slave_dropdown.currentIndex()]
            selected_reg = self.register_dropdown.currentText()

            if selected_slave in data_buffer and selected_reg in data_buffer[selected_slave]:
                processed = data_buffer[selected_slave][selected_reg]

        return processed

    def update_plot(self):
        data = self.process_data()
        if not data:
            return

        # Extract iteration numbers, voltage values, and timestamps
        iterations, values, timestamps = zip(*data)
        iterations = np.array(iterations)
        values = np.array(values)
        timestamps = np.array(timestamps)

        # Downsample data for smoother visualization
        iterations, values = self.downsample_data(iterations, values)

        # Calculate sample rate (samples per second) for the selected register
        current_time = time.time()
        recent_samples = [t for t in timestamps if current_time - t <= 1]  # Samples in the last second
        sample_rate = len(recent_samples)
        self.sample_rate_label.setText(f"Sample Rate: {sample_rate} Hz")

        # Calculate total data rate (all registers for the selected slave)
        with lock:
            selected_slave = self.slave_ids[self.slave_dropdown.currentIndex()]
            if selected_slave in data_buffer:
                all_timestamps = []
                for reg_data in data_buffer[selected_slave].values():
                    all_timestamps.extend([t for _, _, t in reg_data])
                recent_total_samples = [t for t in all_timestamps if current_time - t <= 1]
                total_data_rate = len(recent_total_samples)
                self.total_data_rate_label.setText(f"Total Data Rate: {total_data_rate} Hz")

        # Clear previous trace
        if self.current_trace is not None:
            self.plot_widget.removeItem(self.current_trace)

        # Plot new trace
        self.current_trace = self.plot_widget.plot(iterations, values, pen=pg.mkPen('b', width=3))

        # Update x-axis range smoothly
        self.plot_widget.setXRange(iterations[-1] - MAX_ITERATIONS, iterations[-1], padding=0.02)

    def start_data(self):
        global data_running
        baudrate = int(self.baudrate_dropdown.currentText())
        self.master.baudrate = baudrate
        data_running = True
        logging.info(f"Data transmission started at {baudrate} baud")

    def stop_data(self):
        global data_running
        data_running = False
        logging.info("Data transmission stopped")


def main():
    slave_ids = [1, 2, 3, 4]

    global data_buffer
    data_buffer = {sid: {reg: [] for reg in ["A0", "A1", "A2", "A3"]} for sid in slave_ids}
    master = ModbusRTUMaster(
        port="COM6",
        slave_ids=slave_ids,
        baudrate=115200,
        timeout=0.05,  # Reduced timeout for faster responses
    )

    shutdown_event = threading.Event()
    modbus_thread = threading.Thread(target=lambda: asyncio.run(modbus_main(master, shutdown_event)))
    modbus_thread.start()

    app = QtWidgets.QApplication(sys.argv)
    viewer = AccumulatedPlotViewer(slave_ids)
    viewer.master = master  # Simpan instance ModbusRTUMaster di viewer
    viewer.show()

    exit_code = app.exec_()

    shutdown_event.set()
    modbus_thread.join()
    logging.info("Application shutdown complete")
    sys.exit(exit_code)


if __name__ == '__main__':
    main()