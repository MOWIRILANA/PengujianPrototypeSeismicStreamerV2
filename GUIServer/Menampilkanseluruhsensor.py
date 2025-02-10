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
from PyQt5.QtWidgets import QMenu

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
        self.setGeometry(100, 100, 1000, 800)  # Increased height to accommodate text display

        main_widget = QtWidgets.QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QtWidgets.QVBoxLayout(main_widget)  # Changed to QVBoxLayout for vertical stacking
        # Add a layout for text display
        text_layout = QtWidgets.QHBoxLayout(main_widget)
        main_layout.addLayout(text_layout)  # Add text layout to the main layout

        # Plot widget
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.plot_widget.setLabel('left', 'Voltage (V)')
        self.plot_widget.setLabel('bottom', 'Iteration')

        # Control panel
        control_panel = QtWidgets.QWidget()
        control_layout = QtWidgets.QHBoxLayout(control_panel)  # Changed to QHBoxLayout for horizontal stacking

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

        # Text display for latest data
        self.text_display = QtWidgets.QPlainTextEdit()
        self.text_display.setReadOnly(True)
        self.text_display.setStyleSheet("font-family: monospace;")
        self.text_display.setMinimumHeight(150)  # Set a minimum height for the text display

        # Text display for latest data
        self.text_display_Right = QtWidgets.QPlainTextEdit()
        self.text_display_Right.setReadOnly(True)
        self.text_display_Right.setStyleSheet("font-family: monospace;")
        self.text_display_Right.setMinimumHeight(150)  # Set a minimum height for the text display

        # Add widgets to main_layout
        main_layout.addWidget(self.plot_widget, stretch=4)  # Plot takes 4 parts of the space
        main_layout.addWidget(control_panel, stretch=1)  # Control panel takes 1 part of the space
        text_layout.addWidget(self.text_display, stretch=1)  # Text display takes 1 part of the space
        text_layout.addWidget(self.text_display_Right, stretch=1)  # Text display takes 1 part of the space

    def init_data(self):
        self.current_trace = None

    def start_timers(self):
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(50)  # Update plot every 50 ms

        self.text_timer = QtCore.QTimer()
        self.text_timer.timeout.connect(self.update_text_display)
        self.text_timer.timeout.connect(self.update_text_display_Right)
        self.text_timer.start(1000)  # Update text display every 1000 ms

    def downsample_data(self, x, y, max_points=500):
        """Downsample data to reduce the number of points while preserving the shape."""
        if len(x) > max_points:
            indices = np.linspace(0, len(x) - 1, max_points).astype(int)
            return x[indices], y[indices]
        return x, y

    def process_data(self):
        global data_buffer
        processed_data = {}

        with lock:
            for slave_id in self.slave_ids:
                if slave_id in data_buffer:
                    for reg in ["A0", "A1", "A2", "A3"]:
                        if reg in data_buffer[slave_id]:
                            processed_data[f"Slave {slave_id} {reg}"] = data_buffer[slave_id][reg]

        return processed_data

    def update_plot(self):
        processed_data = self.process_data()
        if not processed_data:
            return

        # Clear previous traces
        if hasattr(self, 'traces'):
            for trace in self.traces:
                self.plot_widget.removeItem(trace)
        self.traces = []

        # Define colors for each trace
        colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k', 'w', 
                '#FF5733', '#33FF57', '#3357FF', '#F333FF', 
                '#FF3333', '#33FF33', '#3333FF', '#FF33FF']

        # Plot each trace
        for idx, (key, data) in enumerate(processed_data.items()):
            if data:
                iterations, values, timestamps = zip(*data)
                iterations = np.array(iterations)
                values = np.array(values)

                # Downsample data for smoother visualization
                iterations, values = self.downsample_data(iterations, values)

                # Plot the trace with a unique color
                trace = self.plot_widget.plot(iterations, values, pen=pg.mkPen(colors[idx % len(colors)], width=1), name=key)
                self.traces.append(trace)

        # Update x-axis range smoothly
        if processed_data:
            last_iteration = max([data[-1][0] for data in processed_data.values()])
            self.plot_widget.setXRange(last_iteration - MAX_ITERATIONS, last_iteration, padding=0.02)

    def update_text_display(self):
        global data_buffer
        text = "Latest Data:\n"
        with lock:
            for slave_id in [1, 2]:
                text += f"Slave {slave_id}:\n"
                if slave_id in data_buffer:
                    for reg in ["A0", "A1", "A2", "A3"]:
                        if reg in data_buffer[slave_id] and data_buffer[slave_id][reg]:
                            last_value = data_buffer[slave_id][reg][-1][1]  # Get the latest voltage value
                            text += f"  {reg}: {last_value:.3f} V\n"
                else:
                    text += "  No data available\n"
        self.text_display.setPlainText(text)

    def update_text_display_Right(self):
        global data_buffer
        text = "Latest Data:\n"
        with lock:
            for slave_id in [3, 4]:
                text += f"Slave {slave_id}:\n"
                if slave_id in data_buffer:
                    for reg in ["A0", "A1", "A2", "A3"]:
                        if reg in data_buffer[slave_id] and data_buffer[slave_id][reg]:
                            last_value = data_buffer[slave_id][reg][-1][1]  # Get the latest voltage value
                            text += f"  {reg}: {last_value:.3f} V\n"
                else:
                    text += "  No data available\n"
        self.text_display_Right.setPlainText(text)

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