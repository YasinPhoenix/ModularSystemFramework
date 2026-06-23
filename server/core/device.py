import time
from collections import deque

class Device:
    def __init__(self, mac, conn, addr):
        self.mac = mac
        self.conn = conn
        self.addr = addr

        self.first_seen = time.time()
        self.last_seen = time.time()

        self.connected = True
        self.logs = deque(maxlen=200)

    def touch(self):
        self.last_seen = time.time()

    def add_log(self, log):
        self.logs.append(log)
        self.touch()

    def is_dead(self, timeout=30):
        return (time.time() - self.last_seen) > timeout

    def info(self):
        return {
            "mac_address": self.mac,
            "is_connected": self.connected,
            "first_seen": self.first_seen,
            "last_seen": self.last_seen,
            "log_count": len(self.logs),
        }