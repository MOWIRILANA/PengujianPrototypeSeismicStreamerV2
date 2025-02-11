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
selected_slave = None
selected_reg = None

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
    global data_buffer, data_running, selected_slave, selected_reg
    try:
        iteration = 0
        while not shutdown_event.is_set():
            if data_running and selected_slave is not None and selected_reg is not None:
                voltages = await master.read_voltages(selected_slave)
                if voltages:
                    timestamp = datetime.now().timestamp()
                    with lock:
                        if selected_slave not in data_buffer:
                            data_buffer[selected_slave] = {}
                        if selected_reg not in data_buffer[selected_slave]:
                            data_buffer[selected_slave][selected_reg] = []
                        reg_index = ["A0", "A1", "A2", "A3"].index(selected_reg)
                        data_buffer[selected_slave][selected_reg].append((iteration, voltages[reg_index], timestamp))
                        # Limit data to MAX_ITERATIONS
                        if len(data_buffer[selected_slave][selected_reg]) > MAX_ITERATIONS:
                            data_buffer[selected_slave][selected_reg].pop(0)
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
        self.setWindowTitle("Realtime Seismic Plot")
        self.setGeometry(100, 100, 1000, 600)

        main_widget = QtWidgets.QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QtWidgets.QHBoxLayout(main_widget)

        # Plot widget
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.plot_widget.setLabel('left', 'Trace Number')
        self.plot_widget.setLabel('bottom', 'Time (ms)')

        # Control panel
        control_panel = QtWidgets.QWidget()
        control_layout = QtWidgets.QVBoxLayout(control_panel)

        # Dropdown untuk memilih slave dan register
        self.slave_dropdown = QtWidgets.QComboBox()
        self.slave_dropdown.addItems([f"Slave {sid}" for sid in self.slave_ids])
        self.register_dropdown = QtWidgets.QComboBox()
        self.register_dropdown.addItems(["A0", "A1", "A2", "A3"])

        # ComboBox untuk memilih baudrate
        self.baudrate_dropdown = QtWidgets.QComboBox()
        self.baudrate_dropdown.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.baudrate_dropdown.setCurrentText("115200")

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
        control_layout.addStretch()

        # Tambahkan plot_widget dan control_panel ke main_layout
        main_layout.addWidget(self.plot_widget, stretch=4)
        main_layout.addWidget(control_panel, stretch=1)

    def init_data(self):
        self.current_trace = None

    def start_timers(self):
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(50)

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
        recent_samples = [t for t in timestamps if current_time - t <= 1]
        sample_rate = len(recent_samples)
        self.sample_rate_label.setText(f"Sample Rate: {sample_rate} Hz")

        # Clear previous traces
        if hasattr(self, 'traces'):
            for trace in self.traces:
                self.plot_widget.removeItem(trace)
        self.traces = []

        # Plot traces vertically
        num_traces = 50
        trace_spacing = 0.1
        for i in range(num_traces):
            shifted_values = values + i * trace_spacing
            trace = self.plot_widget.plot(iterations, shifted_values, pen=pg.mkPen('b', width=1))
            self.traces.append(trace)

        # Update x-axis range to show the latest data
        if len(iterations) > 0:
            x_min = iterations[-1] - MAX_ITERATIONS
            x_max = iterations[-1]
            self.plot_widget.setXRange(x_min, x_max, padding=0.02)

        # Remove data that is outside the visible range
        with lock:
            selected_slave = self.slave_ids[self.slave_dropdown.currentIndex()]
            selected_reg = self.register_dropdown.currentText()
            if selected_slave in data_buffer and selected_reg in data_buffer[selected_slave]:
                data_buffer[selected_slave][selected_reg] = [
                    (iter, val, ts) for iter, val, ts in data_buffer[selected_slave][selected_reg]
                    if iter >= x_min
                ]

    def start_data(self):
        global data_running, selected_slave, selected_reg
        selected_slave = self.slave_ids[self.slave_dropdown.currentIndex()]
        selected_reg = self.register_dropdown.currentText()
        baudrate = int(self.baudrate_dropdown.currentText())
        self.master.baudrate = baudrate
        data_running = True
        logging.info(f"Data transmission started at {baudrate} baud for Slave {selected_slave} and Register {selected_reg}")

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
        timeout=0.05,
    )

    shutdown_event = threading.Event()
    modbus_thread = threading.Thread(target=lambda: asyncio.run(modbus_main(master, shutdown_event)))
    modbus_thread.start()

    app = QtWidgets.QApplication(sys.argv)
    viewer = AccumulatedPlotViewer(slave_ids)
    viewer.master = master
    viewer.show()

    exit_code = app.exec_()

    shutdown_event.set()
    modbus_thread.join()
    logging.info("Application shutdown complete")
    sys.exit(exit_code)


if __name__ == '__main__':
    main()