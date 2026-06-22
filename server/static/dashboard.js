/**
 * Device Dashboard - Frontend Logic
 */

const socket = io();
let selectedDevice = null;
const devices = {};

// ============ DOM Elements ============
const devicesList = document.getElementById("devices-list");
const logsContainer = document.getElementById("logs-container");
const devicesConnectedEl = document.getElementById("devices-connected");
const devicesTotalEl = document.getElementById("devices-total");

// ============ Utility Functions ============

function formatTimestamp(timestamp) {
  const date = new Date(timestamp * 1000);
  return date.toLocaleTimeString("en-US", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}

function formatLogTimestamp(timestamp) {
  const date = new Date(timestamp * 1000);
  const hours = String(date.getHours()).padStart(2, "0");
  const minutes = String(date.getMinutes()).padStart(2, "0");
  const seconds = String(date.getSeconds()).padStart(2, "0");
  return `${hours}:${minutes}:${seconds}`;
}

// ============ WebSocket Events ============

socket.on("connect", () => {
  console.log("Connected to server");
  loadInitialState();
});

socket.on("disconnect", () => {
  console.log("Disconnected from server");
  showMessage("Connection lost", "error");
});

socket.on("devices_updated", (data) => {
  updateDevices(data.devices);
});

socket.on("new_log", (data) => {
  if (data.mac_address === selectedDevice) {
    addLogEntry(data.log);
  }
});

socket.on("new_log_global", (data) => {
  // Could use this for a "all logs" view
});

socket.on("device_status_changed", (data) => {
  console.log(`Device ${data.mac_address} status: ${data.status}`);
  updateDeviceUI(data.mac_address);
});

// ============ API Calls ============

async function loadInitialState() {
  try {
    const response = await fetch("/api/status");
    const data = await response.json();
    updateDevices(data.devices);
  } catch (error) {
    console.error("Failed to load initial state:", error);
    showMessage("Failed to connect to server", "error");
  }
}

async function selectDevice(mac) {
  selectedDevice = mac;
  updateDeviceList();

  // Clear current logs
  logsContainer.innerHTML = "";

  // Load device logs from database
  try {
    const response = await fetch(
      `/api/devices/${mac}/logs?limit=100`
    );
    const data = await response.json();

    // Display logs
    if (data.logs.length === 0) {
      logsContainer.innerHTML =
        '<div class="no-device-selected"><p>No logs yet</p></div>';
    } else {
      logsContainer.innerHTML = "";
      data.logs.forEach((log) => addLogEntry(log));
    }

    // Subscribe to live updates
    socket.emit("subscribe_device", { mac_address: mac });
  } catch (error) {
    console.error("Failed to load device logs:", error);
    showMessage("Failed to load logs", "error");
  }
}

// ============ UI Updates ============

function updateDevices(deviceDict) {
  // Update status counts
  const connected = Object.values(deviceDict).filter(
    (d) => d.is_connected
  ).length;
  const total = Object.keys(deviceDict).length;

  devicesConnectedEl.textContent = connected;
  devicesTotalEl.textContent = total;

  // Update device list
  updateDeviceList(deviceDict);
}

function updateDeviceList(deviceDict) {
  // If called without argument, use existing devices
  if (!deviceDict) {
    deviceDict = devices;
  } else {
    // Update devices object
    Object.assign(devices, deviceDict);
  }

  const deviceItems = Object.entries(deviceDict || {})
    .map(([mac, info]) => ({
      mac,
      info,
    }))
    .sort((a, b) =>
      b.info.is_connected - a.info.is_connected ||
      b.info.last_seen - a.info.last_seen
    );

  devicesList.innerHTML = deviceItems
    .map(({ mac, info }) => {
      const isActive = mac === selectedDevice ? "active" : "";
      const status = info.is_connected ? "online" : "offline";
      const lastSeen = formatTimestamp(info.last_seen);

      return `
      <li class="device-item ${isActive}" onclick="selectDevice('${mac}')">
        <div class="device-name">${mac.split(":").slice(-2).join(":").toUpperCase()}</div>
        <div class="device-mac">${mac}</div>
        <div class="device-status">
          <span class="dot ${status}"></span>
          <span>${status === "online" ? "Connected" : "Offline"}</span>
          <span style="margin-left: auto;">${lastSeen}</span>
        </div>
      </li>
    `;
    })
    .join("");

  // If no device selected and we have devices, select the first one
  if (!selectedDevice && deviceItems.length > 0) {
    selectDevice(deviceItems[0].mac);
  }
}

function addLogEntry(log) {
  // Scroll logs to bottom before adding new entry
  const wasAtBottom =
    logsContainer.scrollHeight -
      logsContainer.clientHeight <=
    logsContainer.scrollTop + 50;

  // Create log element
  const logDiv = document.createElement("div");
  logDiv.className = `log-entry ${log.level}`;

  const timestamp = formatLogTimestamp(log.timestamp);
  logDiv.innerHTML = `
    <span class="log-time">${timestamp}</span>
    <span class="log-level ${log.level}">${log.level}</span>
    <span class="log-message">${escapeHtml(log.message)}</span>
  `;

  logsContainer.appendChild(logDiv);

  // Auto-scroll to bottom if user was at bottom
  if (wasAtBottom) {
    logsContainer.scrollTop = logsContainer.scrollHeight;
  }
}

function updateDeviceUI(mac) {
  // Update specific device in list
  const deviceItem = document.querySelector(
    `.device-item[onclick*="${mac}"]`
  );
  if (deviceItem) {
    const info = devices[mac];
    if (info) {
      const status = info.is_connected ? "online" : "offline";
      const lastSeen = formatTimestamp(info.last_seen);
      deviceItem.querySelector(".device-status").innerHTML = `
        <span class="dot ${status}"></span>
        <span>${status === "online" ? "Connected" : "Offline"}</span>
        <span style="margin-left: auto;">${lastSeen}</span>
      `;
    }
  }
}

function showMessage(message, type) {
  const alertDiv = document.createElement("div");
  alertDiv.style.cssText = `
    position: fixed;
    top: 20px;
    right: 20px;
    background: ${type === "error" ? "#f44336" : "#4caf50"};
    color: white;
    padding: 16px;
    border-radius: 4px;
    z-index: 1000;
  `;
  alertDiv.textContent = message;

  document.body.appendChild(alertDiv);

  setTimeout(() => alertDiv.remove(), 3000);
}

function escapeHtml(text) {
  const map = {
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#039;",
  };
  return text.replace(/[&<>"']/g, (m) => map[m]);
}

// ============ Initialization ============

// Load initial state on page load
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", loadInitialState);
} else {
  loadInitialState();
}
