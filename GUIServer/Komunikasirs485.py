from pymodbus.client import ModbusSerialClient as ModbusClient
import time
import threading

# Konfigurasi koneksi Modbus RTU
SERIAL_PORT = 'COM6'  # Sesuaikan dengan port USB-RS485 Anda
BAUD_RATE = 115200
PARITY = 'N'
STOP_BITS = 1
BYTE_SIZE = 8
TIMEOUT = 1

# Alamat slave Modbus
SLAVE_ID = 2

# Alamat register yang akan dibaca
START_REGISTER = 0
NUM_REGISTERS = 1

# Variabel global untuk menyimpan data
latest_data = None
data_buffer = []  # Buffer untuk menyimpan data setiap 1 detik
data_lock = threading.Lock()  # Lock untuk mengamankan akses ke variabel global

def read_modbus_data():
    global latest_data
    # Inisialisasi client Modbus RTU
    client = ModbusClient(
        port=SERIAL_PORT,
        baudrate=BAUD_RATE,
        parity=PARITY,
        stopbits=STOP_BITS,
        bytesize=BYTE_SIZE,
        timeout=TIMEOUT
    )

    # Membuka koneksi
    if client.connect():
        try:
            while True:
                # Membaca holding registers
                response = client.read_holding_registers(address=START_REGISTER, count=NUM_REGISTERS, slave=SLAVE_ID)

                if response.isError():
                    print(f"Error reading Modbus registers: {response}")
                else:
                    with data_lock:
                        latest_data = response.registers[0]  # Ambil nilai pertama dari list
                        data_buffer.append(latest_data)  # Tambahkan data terbaru ke buffer
                print(f"Data Modbus: {latest_data}")
                time.sleep(0.001)  # 1 ms delay

        except Exception as e:
            print(f"Exception occurred: {e}")

        finally:
            # Menutup koneksi
            client.close()
    else:
        print("Failed to connect to Modbus RTU device")

def collect_data_every_second():
    global data_buffer
    while True:
        with data_lock:
            # Salin data buffer ke variabel sementara untuk diproses
            data_to_process = data_buffer.copy()
            data_buffer = []  # Reset buffer setelah disalin

        # Proses data yang terkumpul dalam 1 detik
        process_data(data_to_process)

        time.sleep(1)  # Tunggu 1 detik

def process_data(data):
    # Fungsi untuk mengolah data yang terkumpul setiap 1 detik
    print(f"Data collected in the last second: {data}")
    # Lakukan pengolahan data di sini, misalnya:
    # - Hitung rata-rata
    # - Simpan ke file
    # - Kirim ke server, dll.

if __name__ == "__main__":
    # Membuat thread untuk membaca data setiap 1 ms
    read_thread = threading.Thread(target=read_modbus_data)
    read_thread.daemon = True
    read_thread.start()

    # Membuat thread untuk mengumpulkan data setiap 1 detik
    collect_thread = threading.Thread(target=collect_data_every_second)
    collect_thread.daemon = True
    collect_thread.start()

    # Menjaga program tetap berjalan
    while True:
        time.sleep(1)