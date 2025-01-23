import asyncio
from pymodbus.client import AsyncModbusSerialClient
from pymodbus.exceptions import ModbusException, ConnectionException
from pymodbus.pdu import ExceptionResponse
import logging
import sys
from datetime import datetime

# Konfigurasi logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('modbus_master.log')
    ]
)

class ModbusRTUMaster:
    def __init__(self, port, slave_ids, baudrate=9600, timeout=3): # Default Baudrate 9600
        self.port = port
        self.slave_ids = slave_ids  # Daftar slave ID
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

async def main():
    # Konfigurasi master
    slave_ids = [2]  # Tambahkan slave ID lainnya jika perlu
    master = ModbusRTUMaster(
        port="COM13",          # Sesuaikan dengan port yang digunakan
        slave_ids=slave_ids,
        baudrate=115200,
        timeout=2
    )

    logging.info("Starting Modbus RTU Master")
    
    try:
        while True:
            for slave_id in slave_ids:
                voltages = await master.read_voltages(slave_id)
                
                if voltages:
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                    logging.info(f"Slave ID: {slave_id} | Time: {timestamp}")
                    logging.info(f"Voltages: A1={voltages[0]:.3f}V, A2={voltages[1]:.3f}V, A3={voltages[2]:.3f}V")
                    
                    # Optional: Save to CSV
                    with open('voltage_data_new.csv', 'a') as f:
                        f.write(f"{timestamp},{slave_id},{voltages[0]:.3f},{voltages[1]:.3f},{voltages[2]:.3f}\n")
                else:
                    logging.warning(f"Failed to read voltages from slave {slave_id}")
            
            await asyncio.sleep(0.1)  # Polling interval

    except KeyboardInterrupt:
        logging.info("Shutting down...")
    except Exception as e:
        logging.error(f"Main loop error: {str(e)}")
    finally:
        await master.disconnect()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Program terminated by user")
