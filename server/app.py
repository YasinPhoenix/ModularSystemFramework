"""Flask web application for device log viewing and management."""

from flask import Flask, render_template, jsonify, request
from flask_socketio import SocketIO, emit, join_room, leave_room
import threading
import time
from server import Server


# Flask app initialization
app = Flask(__name__, template_folder="templates", static_folder="static")
app.config["SECRET_KEY"] = "your-secret-key-change-this"
socketio = SocketIO(app, async_mode="threading")

# Global server instance
server = None
connected_clients = set()


def init_server():
    """Initialize the TCP server."""
    global server
    server = Server(host="0.0.0.0", port=9000, db_path="devices.db")
    server.start()
    print("[APP] TCP Server initialized")


def broadcast_device_update():
    """Broadcast device status to all connected clients."""
    if not server:
        return

    devices = server.get_registry().to_dict()
    socketio.emit("devices_updated", {"devices": devices})


# ============ REST API Routes ============


@app.route("/")
def index():
    """Main dashboard page."""
    return render_template("index.html")


@app.route("/api/devices")
def api_get_devices():
    """Get all devices with current status."""
    if not server:
        return jsonify({"error": "Server not initialized"}), 500

    devices = server.get_registry().to_dict()
    return jsonify({"devices": devices})


@app.route("/api/devices/<mac_address>")
def api_get_device(mac_address):
    """Get device details."""
    if not server:
        return jsonify({"error": "Server not initialized"}), 500

    device = server.get_registry().get_device(mac_address)

    if not device:
        return jsonify({"error": "Device not found"}), 404

    return jsonify(device.get_info())


@app.route("/api/devices/<mac_address>/logs")
def api_get_device_logs(mac_address):
    """Get logs for a specific device."""
    if not server:
        return jsonify({"error": "Server not initialized"}), 500

    limit = request.args.get("limit", 100, type=int)
    logs = server.get_database().get_device_logs(mac_address, limit=limit)

    return jsonify({"mac_address": mac_address, "logs": logs})


@app.route("/api/logs")
def api_get_all_logs():
    """Get recent logs from all devices."""
    if not server:
        return jsonify({"error": "Server not initialized"}), 500

    limit = request.args.get("limit", 500, type=int)
    mac_address = request.args.get("mac_address")

    logs = server.get_database().get_all_logs(limit=limit, mac_address=mac_address)

    return jsonify({"logs": logs})


@app.route("/api/status")
def api_get_status():
    """Get server status."""
    if not server:
        return jsonify({"error": "Server not initialized"}), 500

    registry = server.get_registry()
    connected = registry.count_connected()
    total = len(registry.get_all_devices())

    return jsonify(
        {
            "server_running": server.running,
            "devices_connected": connected,
            "devices_total": total,
            "devices": registry.to_dict(),
        }
    )


# ============ WebSocket Events ============


@socketio.on("connect")
def handle_connect():
    """Handle client connection."""
    print(f"[WS] Client connected: {request.sid}")
    connected_clients.add(request.sid)

    # Send initial state
    if server:
        devices = server.get_registry().to_dict()
        emit("devices_updated", {"devices": devices})


@socketio.on("disconnect")
def handle_disconnect():
    """Handle client disconnection."""
    print(f"[WS] Client disconnected: {request.sid}")
    connected_clients.discard(request.sid)


@socketio.on("subscribe_device")
def handle_subscribe_device(data):
    """Subscribe to updates from a specific device."""
    mac_address = data.get("mac_address")
    join_room(f"device_{mac_address}")
    print(f"[WS] Client {request.sid} subscribed to {mac_address}")


@socketio.on("unsubscribe_device")
def handle_unsubscribe_device(data):
    """Unsubscribe from device updates."""
    mac_address = data.get("mac_address")
    leave_room(f"device_{mac_address}")
    print(f"[WS] Client {request.sid} unsubscribed from {mac_address}")


def broadcast_device_log(mac_address, log_entry):
    """Broadcast a new log to subscribed clients."""
    socketio.emit(
        "new_log",
        {"mac_address": mac_address, "log": log_entry},
        room=f"device_{mac_address}",
    )

    # Also broadcast to general log stream
    socketio.emit(
        "new_log_global",
        {"mac_address": mac_address, "log": log_entry},
    )


def broadcast_device_status(mac_address, status):
    """Broadcast device status change."""
    socketio.emit(
        "device_status_changed",
        {"mac_address": mac_address, "status": status},
    )


# ============ Background Tasks ============


def background_update_loop():
    """Periodic background updates."""
    while True:
        time.sleep(2)

        if not server or not connected_clients:
            continue

        try:
            # Periodically broadcast device status to all connected clients
            broadcast_device_update()
        except Exception as e:
            print(f"[BG] Error in update loop: {e}")


# ============ App Initialization ============


@app.before_request
def before_request():
    """Initialize server if not already done."""
    global server
    if not server:
        init_server()


@app.teardown_appcontext
def teardown(exception):
    """Cleanup on app shutdown."""
    if server:
        server.stop()


if __name__ == "__main__":
    # Start background update thread
    bg_thread = threading.Thread(target=background_update_loop, daemon=True)
    bg_thread.start()

    print("[APP] Starting Flask application")
    socketio.run(app, host="0.0.0.0", port=5000, debug=False)