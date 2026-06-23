import time
import threading
from protocol.parser import MessageParser


class PingManager:
    """
    Sends PING every N seconds to all connected devices.
    Removes dead connections safely.
    """

    def __init__(self, registry, bus, interval=5):
        self.registry = registry
        self.bus = bus
        self.interval = interval
        self.running = True

    def start(self):
        t = threading.Thread(target=self.loop, daemon=True)
        t.start()

    def loop(self):
        while self.running:
            time.sleep(self.interval)

            timestamp = int(time.time())

            for device in self.registry.connected():
                conn = device.conn

                if not conn:
                    device.connected = False
                    self.bus.emit_status(device.mac, "offline")
                    self.bus.emit_device_update(self.registry.to_dict(include_history=True))
                    continue

                try:
                    msg = MessageParser.make_ping(timestamp)
                    print(msg)
                    conn.sendall(msg.encode())

                except Exception:
                    # hard fail = mark offline
                    device.connected = False
                    try:
                        conn.close()
                    except:
                        pass