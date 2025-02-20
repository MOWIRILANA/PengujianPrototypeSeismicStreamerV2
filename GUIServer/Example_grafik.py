import tkinter as tk
from tkinter import ttk
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time

class RealtimeSeismicGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Realtime Seismic Data Viewer")

        # Create a matplotlib figure
        self.fig, self.ax = plt.subplots()
        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        # Start/Stop buttons
        self.start_button = ttk.Button(root, text="Start", command=self.start_realtime)
        self.start_button.pack(side=tk.LEFT)
        self.stop_button = ttk.Button(root, text="Stop", command=self.stop_realtime)
        self.stop_button.pack(side=tk.LEFT)

        # Variables for realtime data
        self.is_running = False
        self.num_traces = 50  # Ensure exactly 100 traces
        self.num_samples = 100
        self.data = np.zeros((self.num_samples, self.num_traces))
        self.trace_numbers = np.arange(self.num_traces)
        self.line = np.arange(self.num_samples)

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

    def update_data(self):
        """Simulate realtime data and update the plot."""
        while self.is_running:
            # Simulate new seismic data (replace this with real data acquisition)
            new_trace = np.random.randn(self.num_samples) * 10  # Random seismic trace
            print(new_trace)
            self.data = np.roll(self.data, -1, axis=1)  # Shift data to the left
            self.data[:, -1] = new_trace  # Add new trace to the end

            # Update the plot
            self.ax.clear()
            for i in range(self.data.shape[1]):
                self.ax.plot(self.trace_numbers[i] + self.data[:, i], self.line, color='black')
                self.ax.fill_betweenx(self.line, self.trace_numbers[i], self.trace_numbers[i] + self.data[:, i],
                                      where=self.data[:, i] > 0, color='black')

            self.ax.set_xlabel('Trace Number')
            self.ax.set_ylabel('Time/Depth')
            self.ax.set_xlim(-10, self.num_traces + 10)  # Ensure consistent x-axis range
            self.canvas.draw()

            # Simulate a delay (e.g., waiting for new data)
            time.sleep(0.1)

if __name__ == "__main__":
    root = tk.Tk()
    app = RealtimeSeismicGUI(root)
    root.mainloop()
