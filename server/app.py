from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO
import threading

from tcp.server import TCPServer
from tcp.ping_manager import PingManager
from core.registry import DeviceRegistry
from events.bus import EventBus
from core.db import Database
from flask import request

app = Flask(__name__)
socketio = SocketIO(app, async_mode="threading")

db = Database(path="data.sqlite3")
registry = DeviceRegistry(db=db)
bus = EventBus(socketio)

tcp = TCPServer("0.0.0.0", 9000, registry, bus)
pinger = PingManager(registry, bus, interval=5)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/status")
def status():
    all_devices = registry.to_dict(include_history=True)
    total = db.total_devices() if db else len(all_devices)
    connected = len([d for d in all_devices.values() if d["is_connected"]])
    return jsonify({"devices": all_devices, "total_devices": total, "connected": connected})


@app.route("/api/devices")
def api_devices():
    all_devices = registry.to_dict(include_history=True)
    return jsonify({"devices": list(all_devices.values()), "total_devices": len(all_devices)})


@app.route("/api/devices/<mac>/logs")
def device_logs(mac):
    # dashboard sends ?limit=100
    try:
        limit = int(request.args.get("limit", 100))
    except Exception:
        limit = 100

    logs = []
    if db:
        logs = db.get_logs(mac, limit=limit)

    # fallback to in-memory logs
    if not logs:
        dev = registry.get(mac)
        if dev:
            # deque is newest at right; return oldest first so the UI can append live events
            logs = list(dev.logs)[-limit:]

    if logs is None:
        return ("Not found", 404)

    return jsonify({"mac_address": mac, "logs": logs})


@app.route("/api/devices/<mac>/logs", methods=["DELETE"])
def delete_device_logs(mac):
    if db:
        db.delete_logs(mac)
    return jsonify({"mac_address": mac, "logs_cleared": True})


@app.route("/api/devices/<mac>", methods=["DELETE"])
def delete_device_record(mac):
    if db:
        db.delete_device(mac)
    return jsonify({"mac_address": mac, "deleted": True})


if __name__ == "__main__":
    tcp.start()
    
    pinger = PingManager(registry, bus, interval=5)
    pinger.start()

    print("[APP] running")
    socketio.run(app, host="0.0.0.0", port=5000)