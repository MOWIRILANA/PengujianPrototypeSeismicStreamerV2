import tkinter as tk
from tkinter import ttk
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time
from pymodbus.client import ModbusSerialClient

class RealtimeSeismicGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Realtime Seismic Data Viewer")

        # Create a matplotlib figure
        self.fig, self.ax = plt.subplots()
        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        # Start/Stop button
        self.start_button = ttk.Button(root, text="Start", command=self.start_realtime)
        self.start_button.pack(side=tk.LEFT)
        self.stop_button = ttk.Button(root, text="Stop", command=self.stop_realtime)
        self.stop_button.pack(side=tk.LEFT)

        # Variables for realtime data
        self.is_running = False
        self.data = np.zeros((4, 100))  # 1 channel dengan 100 sampel
        self.trace_numbers = np.arange(4)  # 1 channel
        self.line = np.arange(100)

        # Modbus client setup
        self.port = 'COM6'  # Simpan port sebagai variabel terpisah
        self.modbus_client = ModbusSerialClient(
            port=self.port,
            baudrate=115200,
            parity='N',  # None
            stopbits=1,
            timeout=1
        )
        self.modbus_connected = self.modbus_client.connect()

        if not self.modbus_connected:
            print("Gagal terhubung ke perangkat Modbus.")
        else:
            print(f"Berhasil terhubung ke perangkat Modbus di port {self.port}.")

    def start_realtime(self):
        """Start the realtime data simulation and plotting."""
        if not self.is_running:
            self.is_running = True
            self.thread = threading.Thread(target=self.update_data)
            self.thread.daemon = True  # Daemonize thread to stop when the main program exits
            self.thread.start()

    def stop_realtime(self):
        """Stop the realtime data simulation."""
        self.is_running = False

    def read_modbus_data(self):
        """Read data from Modbus device."""
        try:
            # Read holding registers (address 0, count 1)
            response = self.modbus_client.read_holding_registers(address=0, count=4, slave=2)
            if not response.isError():
                print(f"Data Modbus diterima: {response.registers}")
                return response.registers
            else:
                print("Modbus read error: Respons mengandung kesalahan.")
                return None
        except Exception as e:
            print(f"Modbus communication error: {e}")
            return None

    def update_data(self):
        """Simulate realtime data and update the plot."""
        while self.is_running:
            # Read data from Modbus device
            modbus_data = self.read_modbus_data()
            if modbus_data:
                # Simulate new seismic data based on Modbus data
                new_trace = np.array(modbus_data) / 1000.0  # Convert mV back to volts

                # Update the data array
                self.data = np.roll(self.data, -1, axis=1)  # Shift data to the left
                self.data[:, -1] = new_trace  # Add new trace to the end

                # Update the plot
                self.ax.clear()
                for i in range(self.data.shape[0]):
                    self.ax.plot(self.trace_numbers[i] + self.data[i, :], self.line, color='black')
                    self.ax.fill_betweenx(self.line, self.trace_numbers[i], self.trace_numbers[i] + self.data[i, :],
                                          where=self.data[i, :] > 0, color='black')

                self.ax.set_xlabel('Channel Number')
                self.ax.set_ylabel('Time/Depth')
                self.canvas.draw()
            else:
                # Jika data Modbus tidak diterima, tampilkan pesan kesalahan
                print("Kesalahan: Tidak ada data Modbus yang diterima. Menunggu data selanjutnya...")

            # Simulate a delay (e.g., waiting for new data)
            time.sleep(0.1)

    def __del__(self):
        """Cleanup Modbus connection."""
        if self.modbus_client:
            self.modbus_client.close()

if __name__ == "__main__":
    root = tk.Tk()
    app = RealtimeSeismicGUI(root)
    root.mainloop()