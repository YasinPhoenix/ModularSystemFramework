from threading import Lock
from .device import Device


class DeviceRegistry:
    def __init__(self, db=None):
        self.devices = {}
        self.lock = Lock()
        self.db = db

    def get_or_create(self, mac, conn, addr):
        with self.lock:
            if mac in self.devices:
                dev = self.devices[mac]

                # IMPORTANT: reconnect replaces socket
                dev.conn = conn
                dev.addr = addr
                dev.connected = True
                dev.touch()

                # persist last seen
                if self.db:
                    try:
                        self.db.upsert_device(mac, ip=addr[0], port=addr[1], last_seen=dev.last_seen)
                    except Exception:
                        pass

                return dev

            dev = Device(mac, conn, addr)
            self.devices[mac] = dev

            # persist new device
            if self.db:
                try:
                    self.db.upsert_device(mac, ip=addr[0], port=addr[1], first_seen=dev.first_seen, last_seen=dev.last_seen)
                except Exception:
                    pass

            return dev

    def get(self, mac):
        return self.devices.get(mac)

    def all(self):
        return list(self.devices.values())

    def connected(self):
        return [d for d in self.devices.values() if d.connected]

    def to_dict(self, include_history=False):
        devices = {m: d.info() for m, d in self.devices.items()}
        if include_history and self.db:
            for d in self.db.get_all_devices():
                mac = d["mac_address"]
                if mac in devices:
                    info = devices[mac]
                    info["first_seen"] = info.get("first_seen", d.get("first_seen"))
                    info["last_seen"] = max(info.get("last_seen", 0), d.get("last_seen") or 0)
                    info["last_ip"] = info.get("last_ip") or d.get("last_ip")
                    info["last_port"] = info.get("last_port") or d.get("last_port")
                else:
                    devices[mac] = {
                        "mac_address": mac,
                        "is_connected": False,
                        "first_seen": d.get("first_seen"),
                        "last_seen": d.get("last_seen"),
                        "last_ip": d.get("last_ip"),
                        "last_port": d.get("last_port"),
                        "log_count": 0,
                    }
        return devices