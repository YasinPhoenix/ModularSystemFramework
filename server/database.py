"""Database interface for persistent log storage."""

import sqlite3
import os
from datetime import datetime
from typing import List, Dict, Optional


class Database:
    """SQLite database for logs and device tracking."""

    def __init__(self, db_path: str = "devices.db"):
        self.db_path = db_path
        self.init_db()

    def init_db(self):
        """Initialize database schema."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        # Devices table
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS devices (
                mac_address TEXT PRIMARY KEY,
                name TEXT,
                first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                is_connected BOOLEAN DEFAULT 0
            )
        """
        )

        # Logs table
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS logs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                mac_address TEXT NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                level TEXT NOT NULL,
                message TEXT NOT NULL,
                FOREIGN KEY (mac_address) REFERENCES devices(mac_address)
            )
        """
        )

        # Events table (for sensor data, WiFi status, etc.)
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                mac_address TEXT NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                event_type TEXT NOT NULL,
                key TEXT,
                value TEXT,
                FOREIGN KEY (mac_address) REFERENCES devices(mac_address)
            )
        """
        )

        # Create indexes for faster queries
        cursor.execute(
            "CREATE INDEX IF NOT EXISTS idx_logs_mac ON logs(mac_address)"
        )
        cursor.execute(
            "CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON logs(timestamp)"
        )
        cursor.execute(
            "CREATE INDEX IF NOT EXISTS idx_events_mac ON events(mac_address)"
        )

        conn.commit()
        conn.close()

    def add_or_update_device(
        self, mac_address: str, name: str = None, is_connected: bool = True
    ):
        """Add or update a device."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            INSERT OR REPLACE INTO devices (mac_address, name, is_connected, last_seen)
            VALUES (?, ?, ?, CURRENT_TIMESTAMP)
        """,
            (mac_address, name or "", is_connected),
        )

        conn.commit()
        conn.close()

    def add_log(self, mac_address: str, level: str, message: str):
        """Store a log entry."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            INSERT INTO logs (mac_address, level, message)
            VALUES (?, ?, ?)
        """,
            (mac_address, level, message),
        )

        conn.commit()
        conn.close()

    def add_event(
        self, mac_address: str, event_type: str, key: str = None, value: str = None
    ):
        """Store an event."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            INSERT INTO events (mac_address, event_type, key, value)
            VALUES (?, ?, ?, ?)
        """,
            (mac_address, event_type, key, value),
        )

        conn.commit()
        conn.close()

    def get_device_logs(self, mac_address: str, limit: int = 100) -> List[Dict]:
        """Get recent logs for a device."""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()

        cursor.execute(
            """
            SELECT id, mac_address, timestamp, level, message
            FROM logs
            WHERE mac_address = ?
            ORDER BY timestamp DESC
            LIMIT ?
        """,
            (mac_address, limit),
        )

        rows = cursor.fetchall()
        conn.close()

        return [dict(row) for row in reversed(rows)]

    def get_all_logs(self, limit: int = 500, mac_address: str = None) -> List[Dict]:
        """Get recent logs from all devices or specific device."""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()

        if mac_address:
            cursor.execute(
                """
                SELECT id, mac_address, timestamp, level, message
                FROM logs
                WHERE mac_address = ?
                ORDER BY timestamp DESC
                LIMIT ?
            """,
                (mac_address, limit),
            )
        else:
            cursor.execute(
                """
                SELECT id, mac_address, timestamp, level, message
                FROM logs
                ORDER BY timestamp DESC
                LIMIT ?
            """,
                (limit,),
            )

        rows = cursor.fetchall()
        conn.close()

        return [dict(row) for row in reversed(rows)]

    def get_devices(self) -> List[Dict]:
        """Get all devices."""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()

        cursor.execute("SELECT * FROM devices")
        rows = cursor.fetchall()
        conn.close()

        return [dict(row) for row in rows]

    def set_device_connected(self, mac_address: str, is_connected: bool):
        """Update device connection status."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            UPDATE devices
            SET is_connected = ?, last_seen = CURRENT_TIMESTAMP
            WHERE mac_address = ?
        """,
            (is_connected, mac_address),
        )

        conn.commit()
        conn.close()

    def get_device_info(self, mac_address: str) -> Optional[Dict]:
        """Get device information."""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()

        cursor.execute("SELECT * FROM devices WHERE mac_address = ?", (mac_address,))
        row = cursor.fetchone()
        conn.close()

        return dict(row) if row else None
