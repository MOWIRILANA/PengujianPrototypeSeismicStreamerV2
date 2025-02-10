import serial
import time
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import sys

# Parameter seismik
num_traces = 4
num_samples = 100

# Inisialisasi array untuk menyimpan data seismik
seismic_data = np.zeros((num_traces, num_samples))

# Daftar warna untuk setiap trace
colors = ['b', 'g', 'r', 'c']  # Biru, Hijau, Merah, Cyan

# Variabel untuk mengontrol pembaruan plot
update_plot_flag = True
ser = None  # Variabel untuk koneksi serial

# Daftar pilihan Baudrate dan Port COM
baudrates = [9600, 19200, 38400, 57600, 115200]
com_ports = ['COM1', 'COM2', 'COM3', 'COM4', 'COM5', 'COM6', 'COM7']

def read_serial():
    """Membaca data dari serial dan mengembalikan list nilai float."""
    while True:
        if ser and ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                try:
                    values = list(map(float, line.split(', ')))  # Ubah string ke float list
                    if len(values) == num_traces:
                        return values, line  # Kembalikan nilai dan data mentah
                    else:
                        print("Error: Incorrect number of values received")
                except ValueError:
                    print("Error: Invalid data format")

def update_plot():
    """Memperbarui plot dengan data baru dari serial."""
    global update_plot_flag
    if update_plot_flag and ser and ser.is_open:
        new_data, raw_data = read_serial()
        print(f"Received data: {new_data}")

        # Geser data lama ke kiri dan masukkan data baru di akhir
        seismic_data[:, :-1] = seismic_data[:, 1:]  # Geser kolom ke kiri
        seismic_data[:, -1] = new_data  # Tambahkan data baru di kolom terakhir

        # Bersihkan plot sebelumnya
        ax.clear()

        # Buat sumbu waktu
        time_axis = np.arange(num_samples)

        # Plot setiap trace dengan warna yang berbeda
        for i in range(num_traces):
            trace = seismic_data[i, :]
            ax.plot(trace + i, time_axis, color=colors[i], linewidth=0.5, label=f'Trace {i}')  # Gunakan warna dari list
            ax.fill_betweenx(time_axis, trace + i, i, where=(trace + i > i), color=colors[i], alpha=0.5)

        # Konfigurasi plot
        ax.set_title('Vertical Seismic Wiggle Plot')
        ax.set_xlabel('Trace Number')
        ax.set_ylabel('Time Samples')
        ax.invert_yaxis()  # Waktu meningkat ke bawah

        # Tambahkan legenda
        ax.legend(loc='upper right')

        # Redraw canvas
        canvas.draw()

        # Update serial monitor
        serial_monitor.insert(tk.END, raw_data + "\n")  # Tambahkan data mentah ke serial monitor
        serial_monitor.see(tk.END)  # Scroll ke bagian bawah

        # Jadwalkan pembaruan plot berikutnya
        root.after(100, update_plot)  # Update setiap 100 ms

def start_plot():
    """Memulai pembaruan plot."""
    global update_plot_flag, ser
    if not ser or not ser.is_open:
        try:
            selected_port = com_port_combobox.get()
            selected_baudrate = int(baudrate_combobox.get())
            ser = serial.Serial(selected_port, selected_baudrate, timeout=1)
            time.sleep(2)
            print(f"Serial port {selected_port} opened successfully at {selected_baudrate} baud")
            update_plot_flag = True
            update_plot()  # Mulai pembaruan plot
        except serial.SerialException as e:
            messagebox.showerror("Error", f"Could not open serial port: {e}")
    else:
        update_plot_flag = True
        update_plot()  # Mulai pembaruan plot

def stop_plot():
    """Menghentikan pembaruan plot."""
    global update_plot_flag
    update_plot_flag = False

def close_serial():
    """Menutup koneksi serial dan menghentikan aplikasi."""
    global update_plot_flag, ser
    update_plot_flag = False  # Hentikan pembaruan plot
    if ser and ser.is_open:
        ser.close()
        print("Serial port closed")
    root.destroy()  # Tutup jendela GUI
    sys.exit()  # Hentikan proses Python

# Buat GUI dengan tkinter
root = tk.Tk()
root.title("Seismic Data Visualization")

# Buat frame utama untuk plot, tombol, dan serial monitor
main_frame = tk.Frame(root)
main_frame.pack(fill=tk.BOTH, expand=True)

# Buat frame untuk plot
plot_frame = tk.Frame(main_frame)
plot_frame.grid(row=0, column=0, sticky="nsew")  # Plot di sebelah kiri

# Buat frame untuk tombol dan ComboBox
control_frame = tk.Frame(main_frame)
control_frame.grid(row=0, column=1, sticky="ns")  # Kontrol di sebelah kanan

# Buat ComboBox untuk memilih Baudrate
baudrate_label = ttk.Label(control_frame, text="Baudrate:")
baudrate_label.pack(side=tk.TOP, padx=10, pady=5)
baudrate_combobox = ttk.Combobox(control_frame, values=baudrates, state="readonly")
baudrate_combobox.pack(side=tk.TOP, padx=10, pady=5)
baudrate_combobox.current(baudrates.index(115200))  # Set default baudrate

# Buat ComboBox untuk memilih Port COM
com_port_label = ttk.Label(control_frame, text="COM Port:")
com_port_label.pack(side=tk.TOP, padx=10, pady=5)
com_port_combobox = ttk.Combobox(control_frame, values=com_ports, state="readonly")
com_port_combobox.pack(side=tk.TOP, padx=10, pady=5)
com_port_combobox.current(0)  # Set default COM port

# Buat tombol Start
start_button = ttk.Button(control_frame, text="Start", command=start_plot)
start_button.pack(side=tk.TOP, padx=10, pady=10)

# Buat tombol Stop
stop_button = ttk.Button(control_frame, text="Stop", command=stop_plot)
stop_button.pack(side=tk.TOP, padx=10, pady=10)

# Buat figure dan axis untuk plot
fig, ax = plt.subplots(figsize=(6, 10))

# Embed plot ke dalam GUI tkinter
canvas = FigureCanvasTkAgg(fig, master=plot_frame)
canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)

# Buat serial monitor (ScrolledText)
serial_monitor = scrolledtext.ScrolledText(main_frame, wrap=tk.WORD, width=50, height=10)
serial_monitor.grid(row=1, column=0, columnspan=2, sticky="ew", padx=10, pady=10)  # Tempatkan di bawah plot dan tombol

# Atur tata letak grid agar plot, tombol, dan serial monitor dapat mengembang
main_frame.grid_rowconfigure(0, weight=1)
main_frame.grid_rowconfigure(1, weight=0)
main_frame.grid_columnconfigure(0, weight=1)
main_frame.grid_columnconfigure(1, weight=0)

# Menutup koneksi serial dan menghentikan aplikasi saat jendela ditutup
root.protocol("WM_DELETE_WINDOW", close_serial)

# Jalankan GUI
root.mainloop()