import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets

# Generate synthetic seismic data
num_traces = 70  # Number of traces
num_samples = 1000  # Number of samples per trace
data = np.random.randn(num_traces, num_samples) * 0.1  # Random noise as synthetic seismic data

# Apply a simple wavelet to make it look more like seismic data
wavelet = np.hanning(50)  # Hanning window as a simple wavelet
for i in range(num_traces):
    data[i, :] = np.convolve(data[i, :], wavelet, mode='same')

# Create the application
app = QtWidgets.QApplication([])

# Create the plot window
win = pg.GraphicsLayoutWidget(show=True, title="Wiggle Plot of Seismic Shot Gather")
win.resize(1000, 600)
win.setWindowTitle('Wiggle Plot')

# Add a plot item
plot = win.addPlot(title="Seismic Shot Gather")
plot.setLabel('left', 'Time (samples)')
plot.setLabel('bottom', 'Trace Number')
plot.showGrid(x=True, y=True, alpha=0.3)

# Plot each trace with a slight horizontal offset
trace_spacing = 0.1  # Spacing between traces
for i in range(num_traces):
    y = data[i, :]  # Seismic data for the current trace
    x = np.full_like(y, i * trace_spacing)  # Horizontal position for the current trace
    plot.plot(x, y, pen=pg.mkPen('b', width=1))

# Run the application
QtWidgets.QApplication.instance().exec_()