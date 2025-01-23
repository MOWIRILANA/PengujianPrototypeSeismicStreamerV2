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

# Logging configuration
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('modbus_master_graph.log')
    ]
)

# Global data storage for each slave
data = {}
lock = threading.Lock()
data_count = 0  # Counter for incoming data
start_time = time.time()  # To track the start time for calculating data rate per second

class ModbusRTUMaster:
    def __init__(self, port, slave_ids, baudrate=9600, timeout=3):
        self.port = port
        self.slave_ids = slave_ids
        self.baudrate = baudrate
        self.timeout = timeout
        self.client = None
        self.connected = False
        self.reconnect_delay = 5  # seconds
        self.max_retries = 3

    async def connect(self):
        """Establish connection with retry mechanism"""
        if self.client and self.client.connected:
            return True

        self.client = AsyncModbusSerialClient(
            port=self.port,
            baudrate=self.baudrate,
            bytesize=8,
            parity="N",
            stopbits=1,
            timeout=self.timeout,
            retries=self.max_retries
        )

        try:
            self.connected = await self.client.connect()
            if self.connected:
                logging.info(f"Successfully connected to {self.port}")
                return True
            logging.error(f"Failed to connect to {self.port}")
            return False
        except Exception as e:
            logging.error(f"Connection error: {str(e)}")
            return False

    async def disconnect(self):
        """Safely disconnect from the client"""
        if self.client and self.client.connected:
            await self.client.close()
            self.connected = False
            logging.info("Disconnected from client")

    def decode_voltages(self, registers):
        """Convert register values to voltages with error checking"""
        try:
            voltages = [reg / 1000.0 for reg in registers]
            return voltages
        except Exception as e:
            logging.error(f"Error decoding voltages: {str(e)}")
            return None

    async def read_voltages(self, slave_id):
        """Read voltage values from a specific slave"""
        for attempt in range(self.max_retries):
            try:
                if not self.connected:
                    connected = await self.connect()
                    if not connected:
                        logging.error(f"Failed to connect (attempt {attempt + 1}/{self.max_retries})")
                        await asyncio.sleep(self.reconnect_delay)
                        continue

                response = await self.client.read_holding_registers(
                    address=0,
                    count=3,
                    slave=slave_id
                )

                if response is None:
                    raise ModbusException("No response from slave")

                if isinstance(response, ExceptionResponse):
                    raise ModbusException(f"Slave {slave_id} returned exception: {response}")

                if not hasattr(response, 'registers'):
                    raise ModbusException("Invalid response format")

                voltages = self.decode_voltages(response.registers)
                if voltages is None:
                    raise ModbusException("Failed to decode voltages")

                return voltages

            except ConnectionException as e:
                logging.error(f"Connection error with slave {slave_id} (attempt {attempt + 1}/{self.max_retries}): {str(e)}")
                self.connected = False
                await asyncio.sleep(self.reconnect_delay)
            
            except ModbusException as e:
                logging.error(f"Modbus error with slave {slave_id} (attempt {attempt + 1}/{self.max_retries}): {str(e)}")
                await asyncio.sleep(1)
            
            except Exception as e:
                logging.error(f"Unexpected error with slave {slave_id} (attempt {attempt + 1}/{self.max_retries}): {str(e)}")
                await asyncio.sleep(1)

        return None

async def modbus_main(master):
    global data_count, start_time
    try:
        while True:
            for slave_id in master.slave_ids:
                voltages = await master.read_voltages(slave_id)
                
                if voltages:
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                    logging.info(f"Slave ID: {slave_id} | Time: {timestamp}")
                    logging.info(f"Voltages: A1={voltages[0]:.3f}V, A2={voltages[1]:.3f}V, A3={voltages[2]:.3f}V")
                    
                    with lock:
                        if slave_id not in data:
                            data[slave_id] = {"x": [], "A1": [], "A2": [], "A3": []}

                        x = data[slave_id]["x"][-1] + 1 if data[slave_id]["x"] else 0
                        data[slave_id]["x"].append(x)
                        data[slave_id]["A1"].append(voltages[0])
                        data[slave_id]["A2"].append(voltages[1])
                        data[slave_id]["A3"].append(voltages[2])

                        # Increment the data count for each data point received
                        data_count += 1

                        # Limit the data to 500 points
                        max_points = 500
                        for key in ["x", "A1", "A2", "A3"]:
                            data[slave_id][key] = data[slave_id][key][-max_points:]

            await asyncio.sleep(0.01)  # Polling interval
    except Exception as e:
        logging.error(f"Error in Modbus loop: {str(e)}")

def pyqtgraph_main(slave_ids):
    global data, data_count, start_time, last_update_time

    # Inisialisasi data rate counter untuk setiap sinyal
    data_rate_counters = {key: 0 for key in ["A0", "A1", "A2", "A3"]}
    last_update_time = time.time()  # Set waktu awal

    # Pastikan QApplication dibuat di thread utama
    app = QtWidgets.QApplication([])
    win = pg.GraphicsLayoutWidget(show=True, title="Real-Time Data")
    plots = {}
    curves = {}

    # Colors for each signal
    colors = {"A0": 'r', "A1": 'g', "A2": 'b', "A3": 'y'}

    # Setup plot for all signals
    plot = win.addPlot(title="Signal Data (A0, A1, A2, A3)")
    plot.addLegend()
    curves = {
        "A0": plot.plot(pen=colors["A0"], name="A0"),
        "A1": plot.plot(pen=colors["A1"], name="A1"),
        "A2": plot.plot(pen=colors["A2"], name="A2"),
        "A3": plot.plot(pen=colors["A3"], name="A3")
    }

    # Tambahkan label untuk data rates
    data_rate_labels = {}
    for idx, key in enumerate(["A0", "A1", "A2", "A3"]):
        label = pg.LabelItem(justify='right')
        data_rate_labels[key] = label
        win.addItem(label, row=idx + 1, col=0)  # Set posisi unik untuk setiap label

    avg_rate_label = pg.LabelItem(justify='right')
    win.addItem(avg_rate_label, row=5, col=0)  # Posisi untuk rata-rata keseluruhan

    def update():
        global data, last_update_time, data_rate_counters

        current_time = time.time()
        time_elapsed = current_time - last_update_time

        with lock:
            # Update graphs
            if data:
                for key in ["A0", "A1", "A2", "A3"]:
                    if key in data and data[key]["x"]:
                        curves[key].setData(data[key]["x"], data[key]["values"])

                # Update data rates sekali per detik
                if time_elapsed >= 1.0:
                    # Hitung data rate untuk setiap sinyal
                    avg_rate = 0
                    total_signals = len(data_rate_counters)

                    for key in data_rate_counters:
                        data_rate_counters[key] = len(data[key]["x"]) / time_elapsed if key in data and data[key]["x"] else 0
                        avg_rate += data_rate_counters[key]
                        data_rate_labels[key].setText(f"Data Rate ({key}): {int(data_rate_counters[key])} data/sec")

                    # Hitung rata-rata keseluruhan
                    avg_rate /= total_signals
                    avg_rate_label.setText(f"Overall Avg Data Rate: {avg_rate:.2f} data/sec")

                    # Reset waktu
                    last_update_time = current_time

    # Start the timer with 10 ms interval
    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(10)

    app.exec_()


if __name__ == "__main__":
    slave_ids = [2]  # Tambahkan slave ID sesuai kebutuhan
    master = ModbusRTUMaster(
        port="COM13",  # Sesuaikan port
        slave_ids=slave_ids,
        baudrate=115200,
        timeout=2
    )

    # Start Modbus loop in a separate thread
    modbus_thread = threading.Thread(target=lambda: asyncio.run(modbus_main(master)))
    modbus_thread.start()

    # Run PyQtGraph in the main thread
    pyqtgraph_main(slave_ids)
