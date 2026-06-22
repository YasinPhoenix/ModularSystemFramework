"""Main TCP server for device connections and log aggregation."""

import socket
import threading
import time
import sys
from device import DeviceRegistry
from message_parser import MessageParser
from ping_manager import PingManager
from database import Database


class Server:
    """TCP server for receiving logs and data from ESP32 devices."""

    def __init__(self, host: str = "0.0.0.0", port: int = 9000, db_path: str = "devices.db"):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.listener_thread = None

        self.registry = DeviceRegistry()
        self.database = Database(db_path)
        self.ping_manager = PingManager(
            self.registry,
            ping_interval=10,
            timeout=30,
            on_device_timeout=self._on_device_timeout,
        )

        # Load devices from database
        self._load_devices_from_db()

    def _load_devices_from_db(self):
        """Load known devices from database on startup."""
        devices = self.database.get_devices()
        print(f"[SERVER] Loaded {len(devices)} known devices from database")

    def start(self):
        """Start the server."""
        if self.running:
            print("[SERVER] Server already running")
            return

        self.running = True

        # Create socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        self.socket.listen(5)

        print(f"[SERVER] Listening on {self.host}:{self.port}")

        # Start listener thread
        self.listener_thread = threading.Thread(target=self._accept_loop, daemon=False)
        self.listener_thread.start()

        # Start ping manager
        self.ping_manager.start()

        print("[SERVER] Started successfully")

    def stop(self):
        """Stop the server."""
        self.running = False
        self.ping_manager.stop()

        if self.socket:
            try:
                self.socket.close()
            except:
                pass

        if self.listener_thread:
            self.listener_thread.join(timeout=5)

        print("[SERVER] Stopped")

    def _accept_loop(self):
        """Accept incoming connections."""
        while self.running:
            try:
                conn, addr = self.socket.accept()
                print(f"[CONNECTION] New connection from {addr}")

                # Start reader thread for this device
                reader_thread = threading.Thread(
                    target=self._handle_device,
                    args=(conn, addr),
                    daemon=True,
                )
                reader_thread.start()

            except OSError:
                # Socket closed
                break
            except Exception as e:
                print(f"[SERVER] Accept error: {e}")
                time.sleep(0.1)

    def _handle_device(self, conn: socket.socket, addr: tuple):
        """Handle a single device connection."""
        mac_address = None
        device = None
        buffer = ""

        try:
            # Send IDENTIFY request
            conn.sendall(MessageParser.make_identify_request().encode())

            # Read data from device
            while self.running:
                try:
                    data = conn.recv(4096)

                    if not data:
                        # Connection closed
                        print(f"[{addr}] Connection closed")
                        break

                    # Append to buffer and process complete lines
                    buffer += data.decode("utf-8", errors="ignore")

                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        self._process_message(line, device)

                except socket.timeout:
                    # Timeout waiting for data
                    pass
                except Exception as e:
                    print(f"[{addr}] Read error: {e}")
                    break

        except Exception as e:
            print(f"[{addr}] Handler error: {e}")

        finally:
            # Cleanup
            if device:
                print(f"[{device.mac_address}] Disconnected from {addr}")
                self.registry.mark_disconnected(device.mac_address)
                self.database.set_device_connected(device.mac_address, False)

            try:
                conn.close()
            except:
                pass

    def _process_message(self, line: str, device):
        """Process a message from a device."""
        if not line.strip():
            return

        parsed = MessageParser.parse_message(line)

        if not parsed:
            print(f"[PARSE] Invalid message: {line}")
            return

        msg_type = parsed.get("type")
        mac_address = parsed.get("mac_address")

        # Handle IDENTIFY response
        if msg_type == "IDENTIFY":
            device = self.registry.add_or_reconnect(mac_address, None, None)
            device_name = parsed.get("name", "")

            # Update database
            self.database.add_or_update_device(mac_address, device_name, True)

            # If reconnecting, log it
            if len(self.registry.get_all_devices()) > 1:
                print(f"[{mac_address}] Identified (reconnected)")
            else:
                print(f"[{mac_address}] Identified (new device)")

            device.add_event("CONNECTED", "status", "identified")
            return

        # For other message types, we need a device to be identified
        if not device or device.mac_address != mac_address:
            device = self.registry.get_device(mac_address)

            if not device:
                print(
                    f"[PARSE] Received {msg_type} from unknown device {mac_address}"
                )
                return

        # Handle LOG message
        if msg_type == "LOG":
            level = parsed.get("level", "INFO")
            message = parsed.get("message", "")
            entry = device.add_log(level, message)

            # Store in database
            self.database.add_log(mac_address, level, message)

            print(f"[{mac_address}] {level}: {message}")

        # Handle DATA message
        elif msg_type == "DATA":
            key = parsed.get("key", "")
            value = parsed.get("value", "")
            device.add_event("DATA", key, value)

            # Store in database
            self.database.add_event(mac_address, "DATA", key, value)

            print(f"[{mac_address}] DATA {key}={value}")

        # Handle PONG response
        elif msg_type == "PONG":
            self.ping_manager.handle_pong(mac_address)
            print(f"[{mac_address}] PONG")

    def _on_device_timeout(self, mac_address: str):
        """Called when a device times out."""
        device = self.registry.get_device(mac_address)
        if device:
            device.add_event("TIMEOUT", "reason", f"no_response_for_30s")
            self.database.set_device_connected(mac_address, False)
            print(f"[{mac_address}] Marked as disconnected (timeout)")

    def get_registry(self):
        """Get device registry."""
        return self.registry

    def get_database(self):
        """Get database instance."""
        return self.database


def main():
    """Main entry point."""
    server = Server(host="0.0.0.0", port=9000)

    try:
        server.start()

        # Keep running
        while server.running:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n[SERVER] Shutting down...")
        server.stop()


if __name__ == "__main__":
    main()
