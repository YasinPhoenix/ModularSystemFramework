import threading
import time
from protocol.parser import MessageParser

class ConnectionHandler(threading.Thread):
    def __init__(self, conn, addr, registry, bus):
        super().__init__(daemon=True)
        self.conn = conn
        self.addr = addr
        self.registry = registry
        self.bus = bus
        self.mac = None
        self.running = True
        self.buffer = ""

    def run(self):
        try:
            while self.running:
                data = self.conn.recv(1024)
                if not data:
                    break

                self.buffer += data.decode(errors="ignore")

                while "\n" in self.buffer:
                    line, self.buffer = self.buffer.split("\n", 1)
                    self.handle(line.strip())

        except Exception as e:
            print("[TCP] error:", e)

        finally:
            self.cleanup()

    def handle(self, line):
        msg = MessageParser.parse_message(line)
        if not msg:
            return

        if msg["type"] == "IDENTIFY":
            self.mac = msg["mac_address"]
            dev = self.registry.get_or_create(self.mac, self.conn, self.addr)
            self.bus.emit_status(self.mac, "online")
            self.bus.emit_device_update(self.registry.to_dict(include_history=True))

        elif msg["type"] == "LOG":
            dev = self.registry.get(self.mac)
            if not dev:
                return

            log = {
                "timestamp": time.time(),
                "level": msg["level"],
                "message": msg["message"]
            }

            dev.add_log(log)
            # persist to DB if available
            try:
                if getattr(self.registry, "db", None):
                    self.registry.db.insert_log(self.mac, log)
            except Exception:
                pass

            self.bus.emit_log(self.mac, log)
            self.bus.emit_global_log(self.mac, log)

        elif msg["type"] == "PONG":
            dev = self.registry.get(self.mac)
            if dev:
                dev.touch()

    def cleanup(self):
        if self.mac:
            dev = self.registry.get(self.mac)
            if dev:
                dev.connected = False
                self.bus.emit_status(self.mac, "offline")
                self.bus.emit_device_update(self.registry.to_dict(include_history=True))
                try:
                    if getattr(self.registry, "db", None):
                        self.registry.db.upsert_device(self.mac, ip=dev.addr[0] if dev.addr else None, port=dev.addr[1] if dev.addr else None, last_seen=dev.last_seen)
                except Exception:
                    pass

        try:
            self.conn.close()
        except:
            pass