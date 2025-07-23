import serial, threading, tkinter as tk

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200

class VirtualLCD(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Virtual LCD")
        self.configure(bg="black")
        self.text_area = tk.Text(self, fg="lime", bg="black", font=("Courier", 12))
        self.text_area.pack(expand=True, fill='both')
        threading.Thread(target=self.serial_read, daemon=True).start()

    def serial_read(self):
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("[LCD]"):
                    _, rest = line.split("[LCD] ")
                    self.text_area.insert(tk.END, rest + "\n")
                    self.text_area.see(tk.END)

if __name__ == "__main__":
    VirtualLCD().mainloop()
