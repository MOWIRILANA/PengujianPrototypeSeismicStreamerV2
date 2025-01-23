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
    global data
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

                        # Limit the data to 500 points
                        max_points = 500
                        for key in ["x", "A1", "A2", "A3"]:
                            data[slave_id][key] = data[slave_id][key][-max_points:]
                else:
                    logging.warning(f"Failed to read voltages from slave {slave_id}")
            
            await asyncio.sleep(0.01)  # Polling interval
    except Exception as e:
        logging.error(f"Error in Modbus loop: {str(e)}")

def pyqtgraph_thread(slave_ids):
    global data

    app = QtWidgets.QApplication([])
    win = pg.GraphicsLayoutWidget(show=True, title="Real-Time Data")
    plots = {}
    curves = {}

    # Colors for each slave
    colors = {
        1: {"A1": 'r', "A2": 'g', "A3": 'b'},
        2: {"A1": 'c', "A2": 'm', "A3": 'y'}
    }

    for slave_id in slave_ids:
        plot = win.addPlot(title=f"Slave ID {slave_id}")
        plot.addLegend()
        plots[slave_id] = plot

        curves[slave_id] = {
            "A1": plot.plot(pen=colors[slave_id]["A1"], name="A1"),
            "A2": plot.plot(pen=colors[slave_id]["A2"], name="A2"),
            "A3": plot.plot(pen=colors[slave_id]["A3"], name="A3")
        }

        win.nextRow()

    def update():
        with lock:
            for slave_id in data:
                if data[slave_id]["x"]:
                    for key in ["A1", "A2", "A3"]:
                        curves[slave_id][key].setData(data[slave_id]["x"], data[slave_id][key])

    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(50)  # Update every 50 ms

    app.exec_()

if __name__ == "__main__":
    slave_ids = [1, 2]  # Add more slave IDs as needed
    master = ModbusRTUMaster(
        port="COM13",  # Adjust the port
        slave_ids=slave_ids,
        baudrate=115200,
        timeout=2
    )

    # Start PyQtGraph in a separate thread
    graph_thread = threading.Thread(target=pyqtgraph_thread, args=(slave_ids,), daemon=True)
    graph_thread.start()

    try:
        asyncio.run(modbus_main(master))
    except KeyboardInterrupt:
        logging.info("Program terminated by user")
