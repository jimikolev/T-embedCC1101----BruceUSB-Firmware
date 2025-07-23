import tkinter as tk
from tkinter import scrolledtext
import serial
import threading

# Configure your serial port here:
SERIAL_PORT = '/dev/ttyACM0'  # Update as needed
BAUD_RATE = 115200            # Make sure this matches your device

class CC1101ScreenMirror(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("CC1101 Screen Mirror")
        self.geometry("600x400")

        # Text area to show live LCD output
        self.text_area = scrolledtext.ScrolledText(self, wrap=tk.WORD, state='disabled', font=("Consolas", 12))
        self.text_area.pack(fill=tk.BOTH, expand=True)

        # Frame for buttons
        btn_frame = tk.Frame(self)
        btn_frame.pack(pady=5)

        # Directional and enter buttons
        buttons = [
            ("Up", self.send_up),
            ("Down", self.send_down),
            ("Left", self.send_left),
            ("Right", self.send_right),
            ("Enter", self.send_enter),
        ]

        for (text, cmd) in buttons:
            btn = tk.Button(btn_frame, text=text, width=8, command=cmd)
            btn.pack(side=tk.LEFT, padx=5)

        # Serial connection
        self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)

        # Start thread to read serial data
        self.running = True
        threading.Thread(target=self.read_serial, daemon=True).start()

    def append_text(self, text):
        self.text_area.configure(state='normal')
        self.text_area.insert(tk.END, text)
        self.text_area.see(tk.END)
        self.text_area.configure(state='disabled')

    def read_serial(self):
        buffer = ""
        while self.running:
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data

                    # Process full lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        # Only show lines starting with [LCD]
                        if line.startswith("[LCD]"):
                            # Remove prefix and append to text area
                            self.append_text(line[5:].strip() + '\n')

            except Exception as e:
                self.append_text(f"\n[Error reading serial: {e}]\n")

    def send_command(self, cmd):
        try:
            self.ser.write(cmd.encode('utf-8'))
        except Exception as e:
            self.append_text(f"\n[Error sending command: {e}]\n")

    # Define your command strings here
    def send_up(self):
        self.send_command("UP\n")

    def send_down(self):
        self.send_command("DOWN\n")

    def send_left(self):
        self.send_command("LEFT\n")

    def send_right(self):
        self.send_command("RIGHT\n")

    def send_enter(self):
        self.send_command("ENTER\n")

    def on_close(self):
        self.running = False
        self.ser.close()
        self.destroy()

if __name__ == "__main__":
    app = CC1101ScreenMirror()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
