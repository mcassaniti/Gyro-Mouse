#!/usr/bin/env python3
"""
Gyro Mouse Base: Interactive Serial Monitor & CLI Terminal
Communicates directly with ESP32-S3 over /dev/tty... with live keystroke passthrough.
"""

import sys
import os
import time
import glob
import select
import threading
import subprocess
import serial

try:
    import termios
    import tty
    HAS_TERMIOS = True
except ImportError:
    HAS_TERMIOS = False

def find_esp32_port():
    """Auto-detect connected ESP32-S3 port via Vendor ID and firmware handshake."""
    # 1. Check serial-by-id for Espressif device
    by_id = glob.glob('/dev/serial/by-id/usb-Espressif*')
    if by_id:
        try:
            return os.path.realpath(by_id[0])
        except Exception:
            return by_id[0]

    candidate_ports = sorted(glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*'))

    # 2. Query udev for Espressif USB Vendor ID (0x303a)
    for p in candidate_ports:
        try:
            out = subprocess.check_output(['udevadm', 'info', '-q', 'property', '-n', p],
                                          text=True, stderr=subprocess.DEVNULL)
            if 'ID_VENDOR_ID=303a' in out or 'Espressif' in out:
                return p
        except Exception:
            pass

    # 3. Active firmware probe: query candidate ports with 'h\n'
    for p in candidate_ports:
        try:
            s = serial.Serial(p, 115200, timeout=0.2)
            s.dtr = True
            s.rts = True
            s.write(b'h\n')
            s.flush()
            time.sleep(0.1)
            resp = s.read_all().decode('utf-8', errors='ignore')
            s.close()
            if 'GYRO MOUSE BASE' in resp or 'ESP32' in resp:
                return p
        except Exception:
            pass

    return candidate_ports[0] if candidate_ports else None

class SerialMonitor:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.running = True
        self.old_term_settings = None

    def connect(self):
        while self.running:
            active_port = self.port if self.port else find_esp32_port()
            if active_port and os.path.exists(active_port):
                try:
                    self.ser = serial.Serial(active_port, self.baudrate, timeout=0.1)
                    return True
                except (serial.SerialException, OSError):
                    time.sleep(0.5)
            else:
                sys.stdout.write(f"\r\033[K[WAIT] Waiting for ESP32-S3 device ({self.port or 'auto-detect'})...")
                sys.stdout.flush()
                time.sleep(0.5)
        return False

    def reader_thread(self):
        """Continuously read data from serial port and stream to stdout."""
        while self.running:
            if self.ser and self.ser.is_open:
                try:
                    data = self.ser.read(self.ser.in_waiting or 1)
                    if data:
                        text = data.decode('utf-8', errors='replace')
                        if '\x04' in text:
                            # Firmware requested clean session close
                            text = text.replace('\x04', '')
                            if HAS_TERMIOS and sys.stdin.isatty():
                                text = text.replace('\r\n', '\n').replace('\n', '\r\n')
                            sys.stdout.write(text)
                            sys.stdout.flush()
                            self.running = False
                            break
                        if HAS_TERMIOS and sys.stdin.isatty():
                            # In raw terminal mode, newline must be \r\n
                            text = text.replace('\r\n', '\n').replace('\n', '\r\n')
                        sys.stdout.write(text)
                        sys.stdout.flush()
                except (serial.SerialException, OSError):
                    if self.running:
                        sys.stdout.write("\r\n\033[1;31m[DISCONNECTED] Device lost. Reconnecting...\033[0m\r\n")
                        sys.stdout.flush()
                        try:
                            self.ser.close()
                        except Exception:
                            pass
                        self.ser = None
                        self.connect()
            else:
                time.sleep(0.1)

    def run(self):
        is_tty = sys.stdin.isatty() and HAS_TERMIOS
        if is_tty:
            self.old_term_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())

        if not self.connect():
            return

        # Start background reader thread
        t_reader = threading.Thread(target=self.reader_thread, daemon=True)
        t_reader.start()

        # Main loop: capture user keystrokes and send live over serial
        try:
            while self.running:
                if is_tty:
                    r, _, _ = select.select([sys.stdin], [], [], 0.1)
                    if r:
                        char = sys.stdin.read(1)
                        if not char:
                            break

                        # Emergency local disconnect shortcuts: Ctrl+C (\x03), Ctrl+] (\x1d)
                        if char in ('\x03', '\x1d'):
                            break

                        # Forward keypress (including 'q', 'h', 'z', etc.) to firmware CLI
                        if self.ser and self.ser.is_open:
                            try:
                                self.ser.write(char.encode('utf-8'))
                                self.ser.flush()
                            except (serial.SerialException, OSError):
                                pass
                else:
                    # Non-interactive / piped input fallback
                    line = sys.stdin.readline()
                    if not line:
                        break
                    if self.ser and self.ser.is_open:
                        self.ser.write(line.encode('utf-8'))
                        self.ser.flush()
        except KeyboardInterrupt:
            pass
        finally:
            self.running = False
            if self.ser:
                try:
                    self.ser.close()
                except Exception:
                    pass
            if is_tty and self.old_term_settings:
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_term_settings)
            print("\n\033[1;32m[MONITOR] Closed session.\033[0m")

def main():
    target_port = sys.argv[1] if len(sys.argv) > 1 else None
    monitor = SerialMonitor(port=target_port)
    monitor.run()

if __name__ == '__main__':
    main()
