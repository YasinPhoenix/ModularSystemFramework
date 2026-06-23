import socket
import threading
from tcp.connection import ConnectionHandler

class TCPServer:
    def __init__(self, host, port, registry, bus):
        self.host = host
        self.port = port
        self.registry = registry
        self.bus = bus
        self.running = False

    def start(self):
        self.running = True
        threading.Thread(target=self.loop, daemon=True).start()

    def loop(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((self.host, self.port))
        s.listen()

        print("[TCP] listening on", self.port)

        while self.running:
            conn, addr = s.accept()
            print("[TCP] connection:", addr)

            handler = ConnectionHandler(
                conn, addr, self.registry, self.bus
            )
            handler.start()