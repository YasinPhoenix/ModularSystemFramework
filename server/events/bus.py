from flask_socketio import SocketIO

class EventBus:
    def __init__(self, socketio: SocketIO):
        self.socketio = socketio

    def emit_device_update(self, devices_dict):
        self.socketio.emit("devices_updated", {"devices": devices_dict})

    def emit_log(self, mac, log):
        self.socketio.emit("new_log", {"mac_address": mac, "log": log})

    def emit_global_log(self, mac, log):
        self.socketio.emit("new_log_global", {"mac_address": mac, "log": log})

    def emit_status(self, mac, status):
        self.socketio.emit("device_status_changed", {
            "mac_address": mac,
            "status": status
        })