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
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("modbus_master_graph.log"),
    ],
)

# Global variables
data = {}  # Store data for each slave
lock = threading.Lock()
data_count = 0  # Total count of incoming data
start_time = time.time()  # Track start time for data rate calculation
register_counts = {"A0": 0, "A1": 0, "A2": 0, "A3": 0}  # Track counts for each register

class ModbusRTUMaster:
    def __init__(self, port, slave_ids, baudrate=9600, timeout=3):
        self.port = port
        self.slave_ids = slave_ids
        self.baudrate = baudrate
        self.timeout = timeout
        self.client = None
        self.connected = False

    async def connect(self):
        """Establish connection to Modbus RTU device."""
        if self.client and self.client.connected:
            return True

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
            logging.info(f"Connected to {self.port}")
        else:
            logging.error(f"Failed to connect to {self.port}")
        return self.connected

    async def disconnect(self):
        """Disconnect safely from Modbus RTU device."""
        if self.client and self.client.connected:
            await self.client.close()
            self.connected = False
            logging.info("Disconnected from Modbus RTU device")

    async def read_voltages(self, slave_id):
        """Read holding registers from a specific slave."""
        try:
            if not self.connected:
                await self.connect()

            response = await self.client.read_holding_registers(
                address=0, count=4, slave=slave_id
            )

            if isinstance(response, ExceptionResponse):
                raise ModbusException(f"Slave {slave_id} returned exception: {response}")

            if hasattr(response, "registers"):
                # Decode values into voltages
                voltages = [value / 1000.0 for value in response.registers]
                return voltages
            else:
                raise ModbusException("Invalid response format")
        except (ConnectionException, ModbusException) as e:
            logging.error(f"Error reading from slave {slave_id}: {str(e)}")
            self.connected = False
        except Exception as e:
            logging.error(f"Unexpected error: {str(e)}")
        return None


async def modbus_main(master):
    """Main loop for reading data from slaves."""
    global data_count, start_time, register_counts
    try:
        while True:
            for slave_id in master.slave_ids:
                voltages = await master.read_voltages(slave_id)
                if voltages:
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                    logging.info(
                        f"Slave ID: {slave_id} | Time: {timestamp} | Voltages: {voltages}"
                    )

                    with lock:
                        if slave_id not in data:
                            data[slave_id] = {"x": [], "A0": [], "A1": [], "A2": [], "A3": []}

                        x = data[slave_id]["x"][-1] + 1 if data[slave_id]["x"] else 0
                        data[slave_id]["x"].append(x)
                        for i, key in enumerate(["A0", "A1", "A2", "A3"]):
                            data[slave_id][key].append(voltages[i])
                            register_counts[key] += 1  # Increment count for each register

                        # Keep only the last 500 points
                        max_points = 500
                        for key in ["x", "A0", "A1", "A2", "A3"]:
                            data[slave_id][key] = data[slave_id][key][-max_points:]

            await asyncio.sleep(0)  # Polling interval
    except Exception as e:
        logging.error(f"Error in Modbus loop: {str(e)}")


def pyqtgraph_main(slave_ids):
    """Display data using PyQtGraph."""
    global data, data_count, register_counts

    # Create PyQtGraph application
    app = QtWidgets.QApplication([])
    win = pg.GraphicsLayoutWidget(show=True, title="Real-Time Data Visualization")
    win.resize(800, 600)
    win.setWindowTitle("Modbus RTU Master Data Visualization")

    plots = {}
    curves = {}

    # Create a label for data rates
    rate_label = pg.LabelItem(justify="left")
    win.addItem(rate_label, row=1, col=0)  # Place the rate label below the graph area
    
    for slave_id in slave_ids:
        plots[slave_id] = win.addPlot(title=f"Slave {slave_id} Data", row=0, col=0)
        plots[slave_id].addLegend()
        curves[slave_id] = {
            key: plots[slave_id].plot(pen=pg.mkPen(color), name=key)
            for key, color in zip(["A0", "A1", "A2", "A3"], ["r", "g", "b", "y"])
        }

    def update():
        """Update plots with new data and calculate data rates."""
        global register_counts, start_time
        with lock:
            for slave_id in slave_ids:
                if slave_id in data:
                    for key in ["A0", "A1", "A2", "A3"]:
                        curves[slave_id][key].setData(data[slave_id]["x"], data[slave_id][key])

            # Calculate data rates for each register
            current_time = time.time()
            elapsed_time = current_time - start_time
            if elapsed_time >= 1:
                total_data = sum(register_counts.values())
                avg_rate = total_data / len(register_counts)

                # Display the data rate for each register and the overall rate
                rate_label.setText(
                    f"A0 Rate: {register_counts['A0'] / elapsed_time:.2f} points/s\n"
                    f"A1 Rate: {register_counts['A1'] / elapsed_time:.2f} points/s\n"
                    f"A2 Rate: {register_counts['A2'] / elapsed_time:.2f} points/s\n"
                    f"A3 Rate: {register_counts['A3'] / elapsed_time:.2f} points/s\n"
                    f"Total Rate: {total_data / elapsed_time:.2f} points/s\n"
                )

                # Reset counts and start time for the next second
                register_counts = {key: 0 for key in register_counts}
                start_time = current_time

    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(100)

    app.exec_()


if __name__ == "__main__":
    slave_ids = [3]  # Set slave IDs here
    master = ModbusRTUMaster(
        port="COM15",  # Set the correct COM port
        slave_ids=slave_ids,
        baudrate=115200,
        timeout=2,
    )

    # Run Modbus loop in a separate thread
    modbus_thread = threading.Thread(target=lambda: asyncio.run(modbus_main(master)))
    modbus_thread.start()

    # Start PyQtGraph in the main thread
    pyqtgraph_main(slave_ids)
