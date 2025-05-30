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

        # Set window to maximize
        self.root.state('zoomed')  # For Windows

        # Create a main frame to hold the plot and controls
        self.main_frame = tk.Frame(root)
        self.main_frame.pack(fill=tk.BOTH, expand=True)

        # Create a matplotlib figure
        self.fig, self.ax = plt.subplots()
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.main_frame)
        self.canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Create a control frame for buttons and combobox
        self.control_frame = tk.Frame(self.main_frame)
        self.control_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=10, pady=10)

        # Start/Stop buttons
        self.start_button = ttk.Button(self.control_frame, text="Start", command=self.start_realtime)
        self.start_button.pack(side=tk.TOP, pady=5)
        self.stop_button = ttk.Button(self.control_frame, text="Stop", command=self.stop_realtime)
        self.stop_button.pack(side=tk.TOP, pady=5)

        # Reset Data Plot button
        self.reset_button = ttk.Button(self.control_frame, text="Reset Data Plot", command=self.reset_data)
        self.reset_button.pack(side=tk.TOP, pady=5)

        # Combobox for selecting slave
        self.slave_label = ttk.Label(self.control_frame, text="Select Slave:")
        self.slave_label.pack(side=tk.TOP, pady=5)
        self.slave_combobox = ttk.Combobox(self.control_frame, values=[1,2,3,4], state="readonly")
        self.slave_combobox.current(1)
        self.slave_combobox.pack(side=tk.TOP, pady=5)
        self.slave_combobox.bind("<<ComboboxSelected>>", self.on_slave_change)  # Bind event

        # Combobox for selecting range
        self.range_label = ttk.Label(self.control_frame, text="Select Range:")
        self.range_label.pack(side=tk.TOP, pady=5)
        self.range_combobox = ttk.Combobox(self.control_frame, values=[50, 500, 2000, 10000, 65535], state="readonly")
        self.range_combobox.current(0)  # Set default to 100
        self.range_combobox.pack(side=tk.TOP, pady=5)
        self.range_combobox.bind("<<ComboboxSelected>>", self.on_range_change)  # Bind event

        # Variables for realtime data
        self.is_running = False
        self.num_traces = 200  # Number of traces
        self.num_samples = 30  # Number of samples per trace
        self.data = np.zeros((self.num_samples, self.num_traces))
        self.trace_numbers = np.arange(self.num_traces)
        self.line = np.arange(self.num_samples)
        self.selected_range = 100  # Default range

        # Modbus client setup
        self.client = ModbusSerialClient(
            port='COM6', 
            baudrate=115200, 
            timeout=1, 
            parity='N',
            stopbits=1, 
            bytesize=8)
        
        self.modbus_connected = self.client.connect()
        if not self.modbus_connected:
            print("Failed to connect to Modbus slave")
        else:
            print("Connected to Modbus slave")

        # Buffer untuk menyimpan data Modbus
        self.data_buffer = []
        self.data_lock = threading.Lock()

        # Counter untuk menghitung jumlah data yang diterima setiap detik
        self.data_counter = 0

    def start_realtime(self):
        """Start the realtime data acquisition and plotting."""
        if not self.is_running:
            self.is_running = True
            # Thread untuk membaca data Modbus setiap 1 ms
            self.modbus_thread = threading.Thread(target=self.read_modbus_data)
            self.modbus_thread.daemon = True
            self.modbus_thread.start()
            # Thread untuk memperbarui plot setiap 1 detik
            self.plot_thread = threading.Thread(target=self.update_plot)
            self.plot_thread.daemon = True
            self.plot_thread.start()

    def stop_realtime(self):
        """Stop the realtime data acquisition."""
        self.is_running = False
        self.client.close()

    def reset_data(self):
        """Reset data and stop realtime acquisition."""
        self.stop_realtime()  # Stop the current process
        with self.data_lock:
            self.data_buffer = []  # Clear the buffer
            self.data = np.zeros((self.num_samples, self.num_traces))  # Reset the data array
        self.ax.clear()  # Clear the plot
        self.canvas.draw()  # Redraw the canvas

    def on_slave_change(self, event):
        """Handle slave change event."""
        # self.reset_data()  # Reset data and stop current process
        self.start_realtime()  # Restart with the new slave

    def on_range_change(self, event):
        """Handle range change event."""
        self.selected_range = int(self.range_combobox.get())
        # self.reset_data()  # Reset data and stop current process
        self.start_realtime()  # Restart with the new range

    def read_modbus_data(self):
        """Read Modbus data every 1 ms and store it in the buffer."""
        while self.is_running:
            try:
                if not self.client.connect():
                    print("Failed to connect to Modbus slave")
                    time.sleep(1)
                    continue

                # Ambil nilai slave dari Combobox
                selected_slave = int(self.slave_combobox.get())

                # Baca 1 register mulai dari alamat 0
                response = self.client.read_holding_registers(address=0, count=1, slave=selected_slave)
                if response.isError():
                    print("Modbus read error")
                else:
                    with self.data_lock:
                        # Konversi data Modbus ke data seismic
                        # datamodbus = response.registers[0]
                        datamodbus = (response.registers[0] - 5) / (self.selected_range - 5) * 5 # (data-min)/(max-min)*min
                        # print("Datamodbus:", datamodbus)
                        self.data_buffer.append(datamodbus)  # Simpan data ke buffer
                        self.data_counter += 1  # Increment counter
                        print("Data Register:", response.registers[0])
                        
                time.sleep(0.0001)  # 1 ms delay
            except Exception as e:
                print("Modbus communication error:", e)

    def update_plot(self):
        """Update the plot with data collected in the last second."""
        while self.is_running:
            with self.data_lock:
                if self.data_buffer:
                    # Ambil data dari buffer
                    new_trace = np.array(self.data_buffer)
                    # print("New trace:", new_trace)
                    self.data_buffer = []  # Reset buffer setelah diambil

                    # Pastikan panjang data sesuai dengan num_samples
                    if len(new_trace) < self.num_samples:
                        # Jika data lebih pendek, isi dengan nilai 0
                        new_trace = np.pad(new_trace, (0, self.num_samples - len(new_trace)), mode='constant')
                    else:
                        # Jika data lebih panjang, potong menjadi num_samples
                        new_trace = new_trace[:self.num_samples]

                    # Roll the data to the left and add new trace to the end
                    self.data = np.roll(self.data, -1, axis=1)
                    self.data[:, -1] = new_trace  # Tambahkan data baru ke kolom terakhir

            # Update plot
            self.ax.clear()
            for i in range(self.data.shape[1]):
                self.ax.plot(self.trace_numbers[i] + self.data[:, i], self.line, color='black')  # Garis hitam
                # self.ax.fill_betweenx(self.line, self.trace_numbers[i], self.trace_numbers[i] + self.data[:, i])
                                    # where=self.data[:, i] > 0, color='black', alpha=0.5)  # Area diisi dengan hitam

            self.ax.set_xlabel('Trace Number')
            self.ax.set_ylabel('Time/second')
            self.ax.set_xlim(-10, self.num_traces + 10)
            self.canvas.draw()

            # Tampilkan jumlah data yang diterima setiap detik
            print(f"Data received in the last second: {self.data_counter}")
            self.data_counter = 0  # Reset counter

            time.sleep(1)  # Delay untuk memperbarui plot setiap 1 detik

if __name__ == "__main__":
    root = tk.Tk()
    app = RealtimeSeismicGUI(root)
    root.mainloop()