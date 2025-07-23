import serial
import threading
import tkinter as tk
from tkinter import Canvas
from PIL import Image, ImageTk
import base64
import queue
import re
import os
import struct

# === CONFIG ===
SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
IMAGE_FOLDER = './images'

def int_to_rgb565(color):
    r = ((color >> 11) & 0x1F) << 3
    g = ((color >> 5) & 0x3F) << 2
    b = (color & 0x1F) << 3
    return f'#{r:02x}{g:02x}{b:02x}'

class ScreenMirrorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("T-Embed CC1101 Screen Mirror")
        self.root.geometry("800x480")
        self.root.configure(bg='black')  # fallback
        self.text_elements = {}  # Track text by (x, y) keys
        # Load and place background image using a Label
        try:
            bg_path = os.path.join(os.getcwd(), "bruce_hd.png")
            bg_image = Image.open(bg_path).resize((800, 480), Image.Resampling.LANCZOS)
            self.bg_photo = ImageTk.PhotoImage(bg_image)

            self.background_label = tk.Label(self.root, image=self.bg_photo)
            self.background_label.place(x=0, y=0, relwidth=1, relheight=1)
        except Exception as e:
            print(f"[!] Could not load background: {e}")

        # Main frame sits on top of background
        self.main_frame = tk.Frame(self.root, bg='', highlightthickness=0)
        self.main_frame.place(relx=0.5, rely=0.4, anchor='center')  # use place to layer on top

        # Canvas to show mirrored screen
        self.canvas_width = 320
        self.canvas_height = 170
        self.canvas = tk.Canvas(
            self.main_frame,
            width=self.canvas_width,
            height=self.canvas_height,
            bg='black',
            bd=0,
            highlightthickness=0
        )
        self.canvas.pack(side='left', padx=20, pady=20)

        # Frame for control buttons
        self.button_frame = tk.Frame(self.main_frame, bg='', highlightthickness=0)
        self.button_frame.pack(side='right', padx=30, pady=10)

        self.add_control_buttons()

        # Bottom status banner
        self.banner = tk.Label(
            self.root,
            text="T-Embed CC1101 - BRUCEUSB Predatory-Firmware",
            bg="#0a0a0a",
            fg="white",
            font=("Courier", 12),
            height=2
        )
        self.banner.pack(side='bottom', fill='x')

        # Ensure background is behind all other widgets
        if hasattr(self, 'background_label'):
            self.background_label.lower()

                # --- Start serial reading and queue processing ---
        self.serial_queue = queue.Queue()
        self.running = True
        self.serial_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.serial_thread.start()
        self.root.after(100, self.process_queue)

    def add_control_buttons(self):
        style = {
            "bg": "#2b2b2b",
            "fg": "white",
            "activebackground": "#1e1e1e",
            "activeforeground": "cyan",
            "relief": "flat",
            "width": 4,
            "height": 2,
            "font": ("Arial", 12, "bold"),
            "bd": 1,
        }

        def glow_on_hover(event):
            event.widget.config(highlightbackground="#800080", highlightthickness=2)

        def remove_glow(event):
            event.widget.config(highlightthickness=0)

        buttons = {
            '↑': lambda: self.send_key('UP'),
            '↓': lambda: self.send_key('DOWN'),
            '←': lambda: self.send_key('LEFT'),
            '→': lambda: self.send_key('RIGHT'),
            '😈': lambda: self.send_key('ENTER'),
            '⎋': lambda: self.send_key('ESC'),
            '⏻': lambda: self.send_key('LONG'),
        }

        layout = {
            '⏻': (0, 0),
            '↑':  (0, 1),
            '⎋': (0, 2),
            '←': (1, 0),
            '😈': (1, 1),
            '→': (1, 2),
            '↓': (2, 1),
        }

        for label, command in buttons.items():
            btn = tk.Button(self.button_frame, text=label, command=command, **style)
            btn.grid(row=layout[label][0], column=layout[label][1], padx=6, pady=6)
            btn.bind("<Enter>", glow_on_hover)
            btn.bind("<Leave>", remove_glow)

    def send_key(self, key):
        if hasattr(self, 'ser') and self.ser:
            try:
                self.ser.write((f"[KEY:{key}]\n").encode())
                print(f"[>] Sent key: [KEY:{key}]")
            except Exception as e:
                print(f"[!] Failed to send key: {e}")

    def key_pressed(self, event):
        print(f"[KEY] Pressed: {event.char} (keysym={event.keysym})")

    def read_serial(self):
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
            print(f"[+] Connected to {SERIAL_PORT} @ {BAUD_RATE}")
            while self.running:
                line = self.ser.readline().decode(errors='ignore').strip()
                if line.startswith('[LCD]'):
                    cmd_line = line[5:].strip()
                    self.serial_queue.put(cmd_line)
                elif line.startswith('[FRAMEBUFFER'):
                    parts = line.strip()[1:-1].split()
                    if len(parts) >= 3:
                        width = int(parts[1])
                        height = int(parts[2])
                        expected_size = width * height * 2
                        raw_data = self.ser.read(expected_size)
                        self.serial_queue.put(("FRAMEBUFFER", raw_data, width, height))
                elif line.startswith('[INFO]') or line.startswith('[WIFI]') or line.startswith('[FILES]'):
                    self.serial_queue.put(line)
        except Exception as e:
            print(f"[!] Serial error: {e}")

    def process_queue(self):
        while not self.serial_queue.empty():
            item = self.serial_queue.get()
            if isinstance(item, tuple) and item[0] == "FRAMEBUFFER":
                _, data, width, height = item
                self.render_framebuffer(data, width, height)
            else:
                self.handle_command(item)
        self.root.after(50, self.process_queue)

    def render_framebuffer(self, data, width, height):
        try:
            img = Image.new("RGB", (width, height))
            pixels = img.load()
            for i in range(width * height):
                if i * 2 + 1 >= len(data):
                    break
                value = struct.unpack_from(">H", data, i * 2)[0]
                r = ((value >> 11) & 0x1F) << 3
                g = ((value >> 5) & 0x3F) << 2
                b = (value & 0x1F) << 3
                x = i % width
                y = i // width
                pixels[x, y] = (r, g, b)

            imgtk = ImageTk.PhotoImage(img)
            self.canvas.delete("all")
            self.canvas.create_image(0, 0, anchor="nw", image=imgtk)
            self.canvas.image = imgtk
            self.root.update_idletasks()
            self.root.after(10)
        except Exception as e:
            print(f"[!] Framebuffer render error: {e}")

    def handle_command(self, cmd_line):
        try:
            cmd_line = cmd_line.strip()
            # Handle your new log_print and print commands
            m = re.search(r'(log_print|print)\((\d+),\s*(\d+)\):\s*(.*)', cmd_line, re.IGNORECASE)
            if m:
                cmd_type = m.group(1)
                x = int(m.group(2))
                y = int(m.group(3))
                text = m.group(4)
                if text == '':
                    return

                # Draw the text on canvas at (x, y)
                key = (x, y)
                if key in self.text_elements:
                    self.canvas.delete(self.text_elements[key])
                text_id = self.canvas.create_text(
                    x, y, text=text, fill='white', anchor='nw', font=('Courier', 10)
                )
                self.text_elements[key] = text_id
                self.canvas.tag_raise(text_id)
                return



            if cmd_line.startswith('[INFO]') or cmd_line.startswith('[FILES]') or cmd_line.startswith('[WIFI]'):
                label_color = 'yellow' if cmd_line.startswith('[INFO]') else 'cyan' if cmd_line.startswith('[FILES]') else 'green'
                self.canvas.create_text(5, self.canvas_height - 15, anchor='nw', text=cmd_line, fill=label_color, font=('Courier', 9))
                return

            if cmd_line.lower().startswith("drawstring("):
                match = re.match(r'drawString\((\d+),\s*(\d+)\):\s*(.*)', cmd_line, re.IGNORECASE)
                if match:
                    x, y, text = int(match.group(1)), int(match.group(2)), match.group(3)
                    if text == '':
                        return
                    key = (x, y)
                    if key in self.text_elements:
                        self.canvas.delete(self.text_elements[key])
                    text_id = self.canvas.create_text(
                        x, y, text=text, fill='white', anchor='nw', font=('Courier', 10)
                    )
                    self.text_elements[key] = text_id
                    self.canvas.tag_raise(text_id)
                    return
            if cmd_line.lower().startswith("log_drawstring("):
                match = re.match(r'log_drawString\((\d+),\s*(\d+)\):\s*(.*)', cmd_line, re.IGNORECASE)
                if match:
                    x, y, text = int(match.group(1)), int(match.group(2)), match.group(3)
                    if text == '':
                        return
                    key = (x, y)
                    if key in self.text_elements:
                        self.canvas.delete(self.text_elements[key])
                    text_id = self.canvas.create_text(
                        x, y, text=text, fill='orange', anchor='nw', font=('Courier', 10, 'bold')
                    )
                    self.text_elements[key] = text_id
                    self.canvas.tag_raise(text_id)
                    return


            if cmd_line.lower().startswith("fillscreen:"):
                color = int(cmd_line.split(":")[1].strip())
                self.canvas.delete("all")
                self.canvas.config(bg=int_to_rgb565(color))
                return

            if cmd_line.upper().startswith("BITMAPDATA"):
                return

            parts = cmd_line.split()
            if not parts:
                return

            cmd = parts[0].upper()

            if cmd == 'DRAWLINE' and len(parts) == 6:
                x, y, x1, y1, color = map(int, parts[1:])
                self.canvas.create_line(x, y, x1, y1, fill=int_to_rgb565(color))

            elif cmd == 'DRAWFASTHLINE' and len(parts) == 5:
                x, y, w, color = map(int, parts[1:])
                self.canvas.create_line(x, y, x + w, y, fill=int_to_rgb565(color))
            elif cmd == 'DRAWFASTVLINE' and len(parts) == 5:
                x, y, h, color = map(int, parts[1:])
                self.canvas.create_line(x, y, x, y + h, fill=int_to_rgb565(color))


            elif cmd == 'DRAWWIDELINE' and len(parts) == 8:
                x, y, x1, y1, width, color, _ = map(float, parts[1:])
                self.canvas.create_line(x, y, x1, y1, fill=int_to_rgb565(int(color)), width=width)

            elif cmd == 'DRAWRECT' and len(parts) == 6:
                x, y, w, h, color = map(int, parts[1:])
                self.canvas.create_rectangle(x, y, x + w, y + h, outline=int_to_rgb565(color))

            elif cmd == 'FILLRECT' and len(parts) == 6:
                x, y, w, h, color = map(int, parts[1:])
                self.canvas.create_rectangle(x, y, x + w, y + h, fill=int_to_rgb565(color), outline='')

            elif cmd == 'FILLROUNDRECT' and len(parts) == 7:
                x, y, w, h, r, color = map(int, parts[1:])
                self.draw_rounded_rect(x, y, w, h, r, int_to_rgb565(color), fill=True)

            elif cmd == 'DRAWROUNDRECT' and len(parts) == 7:
                x, y, w, h, r, color = map(int, parts[1:])
                self.draw_rounded_rect(x, y, w, h, r, int_to_rgb565(color), fill=False)

            elif cmd == 'DRAWCIRCLE' and len(parts) == 5:
                x, y, r, color = map(int, parts[1:])
                self.canvas.create_oval(x - r, y - r, x + r, y + r, outline=int_to_rgb565(color))

            elif cmd == 'FILLCIRCLE' and len(parts) == 5:
                x, y, r, color = map(int, parts[1:])
                self.canvas.create_oval(x - r, y - r, x + r, y + r, fill=int_to_rgb565(color), outline='')

            elif cmd == 'DRAWELLIPSE' and len(parts) == 6:
                x, y, rx, ry, color = map(int, parts[1:])
                self.canvas.create_oval(x - rx, y - ry, x + rx, y + ry, outline=int_to_rgb565(color))

            elif cmd == 'DRAWTRIANGLE' and len(parts) == 8:
                x1, y1, x2, y2, x3, y3, color = map(int, parts[1:])
                self.canvas.create_polygon([x1, y1, x2, y2, x3, y3], outline=int_to_rgb565(color), fill='')

            elif cmd == 'FILLTRIANGLE' and len(parts) == 8:
                x1, y1, x2, y2, x3, y3, color = map(int, parts[1:])
                self.canvas.create_polygon([x1, y1, x2, y2, x3, y3], fill=int_to_rgb565(color), outline='')

            elif cmd == 'DRAWARC' and len(parts) == 10:
                x, y, w, h, start, end, color, _, _ = map(int, parts[1:])
                self.canvas.create_arc(x - w, y - h, x + w, y + h, start=start, extent=end - start, style='arc', outline=int_to_rgb565(color))

            elif cmd == 'CLEARSCREEN':
                self.canvas.delete("all")
            else:
                print(f"[?] Unhandled command: {cmd_line}")

        except Exception as e:
            print(f"[!] Error handling '{cmd_line}': {e}")

    def draw_rounded_rect(self, x, y, w, h, r, color, fill=False):
        style = 'pieslice' if fill else 'arc'
        self.canvas.create_arc(x, y, x + 2 * r, y + 2 * r, start=90, extent=90, style=style, outline=color, fill=color if fill else "")
        self.canvas.create_arc(x + w - 2 * r, y, x + w, y + 2 * r, start=0, extent=90, style=style, outline=color, fill=color if fill else "")
        self.canvas.create_arc(x + w - 2 * r, y + h - 2 * r, x + w, y + h, start=270, extent=90, style=style, outline=color, fill=color if fill else "")
        self.canvas.create_arc(x, y + h - 2 * r, x + 2 * r, y + h, start=180, extent=90, style=style, outline=color, fill=color if fill else "")
        if fill:
            self.canvas.create_rectangle(x + r, y, x + w - r, y + h, fill=color, outline=color)
            self.canvas.create_rectangle(x, y + r, x + w, y + h - r, fill=color, outline=color)
        else:
            self.canvas.create_line(x + r, y, x + w - r, y, fill=color)
            self.canvas.create_line(x + w, y + r, x + w, y + h - r, fill=color)
            self.canvas.create_line(x + w - r, y + h, x + r, y + h, fill=color)
            self.canvas.create_line(x, y + h - r, x, y + r, fill=color)

    def stop(self):
        self.running = False
        if hasattr(self, 'ser') and self.ser:
            self.ser.close()

if __name__ == '__main__':
    root = tk.Tk()
    app = ScreenMirrorGUI(root)
    try:
        root.mainloop()
    except KeyboardInterrupt:
        app.stop()
        print("Exiting...")

