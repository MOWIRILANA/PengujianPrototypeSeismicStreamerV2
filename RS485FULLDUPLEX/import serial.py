import serial

ser = serial.Serial('COM6', 9600, timeout=1)  # Ganti COM5 dengan port Anda
while True:
    data = ser.readline().decode('utf-8').strip()
    if data:
        print("Data diterima:", data)
