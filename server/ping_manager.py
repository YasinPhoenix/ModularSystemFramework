"""Ping/Heartbeat management for device connection health monitoring."""

import time
import threading
from typing import Callable, Dict
from message_parser import MessageParser


class PingManager:
    """Manages periodic ping messages and detects dead connections."""

    def __init__(
        self,
        registry,
        ping_interval: int = 10,
        timeout: int = 30,
        on_device_timeout: Callable = None,
    ):
        """
        Initialize PingManager.

        Args:
            registry: DeviceRegistry instance
            ping_interval: Send ping every N seconds
            timeout: Mark device disconnected if no response in N seconds
            on_device_timeout: Callback when device times out
        """
        self.registry = registry
        self.ping_interval = ping_interval
        self.timeout = timeout
        self.on_device_timeout = on_device_timeout
        self.running = False
        self.thread = None
        self.pending_pongs: Dict[str, float] = {}  # mac -> ping_time
        self.lock = threading.Lock()

    def start(self):
        """Start the ping manager thread."""
        if self.running:
            return

        self.running = True
        self.thread = threading.Thread(target=self._ping_loop, daemon=True)
        self.thread.start()

    def stop(self):
        """Stop the ping manager thread."""
        self.running = False
        if self.thread:
            self.thread.join(timeout=5)

    def _ping_loop(self):
        """Main ping loop."""
        while self.running:
            time.sleep(self.ping_interval)

            # Get all connected devices
            devices = self.registry.get_connected_devices()

            for device in devices:
                try:
                    # Send ping message
                    ping_msg = MessageParser.make_ping(int(time.time() * 1000))
                    device.connection.sendall(ping_msg.encode())

                    # Track ping timestamp
                    with self.lock:
                        self.pending_pongs[device.mac_address] = time.time()

                except Exception as e:
                    # Connection failed, mark device as disconnected
                    print(f"[PING] Failed to ping {device.mac_address}: {e}")
                    self.registry.mark_disconnected(device.mac_address)

            # Check for timeout responses
            self._check_timeouts()

    def _check_timeouts(self):
        """Check for devices that didn't respond to ping in time."""
        with self.lock:
            current_time = time.time()
            timed_out = [
                mac
                for mac, ping_time in self.pending_pongs.items()
                if (current_time - ping_time) > self.timeout
            ]

        for mac in timed_out:
            device = self.registry.get_device(mac)
            if device and device.is_connected:
                print(f"[PING] Device {mac} timeout after {self.timeout}s")
                self.registry.mark_disconnected(mac)

                if self.on_device_timeout:
                    self.on_device_timeout(mac)

            with self.lock:
                if mac in self.pending_pongs:
                    del self.pending_pongs[mac]

    def handle_pong(self, mac_address: str):
        """Handle PONG response from device."""
        with self.lock:
            if mac_address in self.pending_pongs:
                del self.pending_pongs[mac_address]

        device = self.registry.get_device(mac_address)
        if device:
            device.update_last_seen()
