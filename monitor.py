import serial
import sys
import time

port = sys.argv[1]
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

while True:
    try:
        s = serial.Serial(port, baud, dsrdtr=False, rtscts=False, timeout=0)
        try:
            s.dtr = True   # nRF52 TinyUSB only writes when DTR=1 (tud_cdc_n_connected)
            s.rts = False
        except Exception:
            pass
        while True:
            data = s.read(256)
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
            else:
                time.sleep(0.005)
    except (serial.SerialException, OSError):
        try:
            s.close()
        except Exception:
            pass
    except KeyboardInterrupt:
        break
    time.sleep(0.05)
