"""Device tracking and registry management."""

import time
from threading import Lock
from collections import deque
from typing import Dict, Optional


class Device:
    """Represents a connected device."""

    def __init__(self, mac_address: str, connection, addr):
        self.mac_address = mac_address
        self.connection = connection
        self.addr = addr
        self.first_seen = time.time()
        self.last_seen = time.time()
        self.is_connected = True
        self.logs = deque(maxlen=200)  # Keep last 200 logs in memory
        self.reader_thread = None

    def update_last_seen(self):
        """Update the last activity timestamp."""
        self.last_seen = time.time()

    def add_log(self, level: str, message: str) -> dict:
        """Add a log entry to device history."""
        entry = {
            "timestamp": time.time(),
            "level": level,
            "message": message,
        }
        self.logs.append(entry)
        self.update_last_seen()
        return entry

    def add_event(self, event_type: str, key: str = None, value: str = None) -> dict:
        """Add a generic event to device history."""
        entry = {
            "timestamp": time.time(),
            "type": "event",
            "event_type": event_type,
            "key": key,
            "value": value,
        }
        self.logs.append(entry)
        self.update_last_seen()
        return entry

    def is_idle(self, timeout_seconds: int = 30) -> bool:
        """Check if device hasn't sent data in timeout_seconds."""
        return (time.time() - self.last_seen) > timeout_seconds

    def disconnect(self):
        """Mark device as disconnected."""
        self.is_connected = False
        if self.connection:
            try:
                self.connection.close()
            except:
                pass

    def get_info(self) -> dict:
        """Get device information."""
        return {
            "mac_address": self.mac_address,
            "is_connected": self.is_connected,
            "first_seen": self.first_seen,
            "last_seen": self.last_seen,
            "idle_time": time.time() - self.last_seen,
            "log_count": len(self.logs),
        }

    def get_recent_logs(self, limit: int = 50) -> list:
        """Get recent logs (newest first)."""
        return list(reversed(list(self.logs)[-limit:]))


class DeviceRegistry:
    """Manages all connected and known devices."""

    def __init__(self):
        self.devices: Dict[str, Device] = {}
        self.lock = Lock()

    def add_or_reconnect(self, mac_address: str, connection, addr) -> Device:
        """
        Add a new device or reconnect an existing one.
        Returns the Device object.
        """
        with self.lock:
            if mac_address in self.devices:
                # Reconnect: update connection, mark as connected
                device = self.devices[mac_address]
                device.connection = connection
                device.addr = addr
                device.is_connected = True
                device.update_last_seen()
            else:
                # New device
                device = Device(mac_address, connection, addr)
                self.devices[mac_address] = device
            return device

    def get_device(self, mac_address: str) -> Optional[Device]:
        """Get a device by MAC address."""
        with self.lock:
            return self.devices.get(mac_address)

    def get_all_devices(self) -> list:
        """Get list of all known devices."""
        with self.lock:
            return list(self.devices.values())

    def get_connected_devices(self) -> list:
        """Get only connected devices."""
        with self.lock:
            return [d for d in self.devices.values() if d.is_connected]

    def mark_disconnected(self, mac_address: str):
        """Mark a device as disconnected (but retain its logs)."""
        with self.lock:
            if mac_address in self.devices:
                self.devices[mac_address].disconnect()

    def remove_device(self, mac_address: str):
        """Completely remove a device from registry."""
        with self.lock:
            if mac_address in self.devices:
                self.devices[mac_address].disconnect()
                del self.devices[mac_address]

    def cleanup_idle(self, timeout_seconds: int = 30):
        """Mark idle devices as disconnected."""
        with self.lock:
            for device in self.devices.values():
                if device.is_connected and device.is_idle(timeout_seconds):
                    device.disconnect()

    def count_connected(self) -> int:
        """Count number of connected devices."""
        with self.lock:
            return sum(1 for d in self.devices.values() if d.is_connected)

    def to_dict(self) -> dict:
        """Convert registry to dictionary for API responses."""
        with self.lock:
            return {
                mac: device.get_info()
                for mac, device in self.devices.items()
            }
