import sqlite3
import threading
import time
from typing import List, Dict


class Database:
    def __init__(self, path=":memory:"):
        self.path = path
        self.lock = threading.Lock()
        self.conn = sqlite3.connect(self.path, check_same_thread=False)
        self.conn.row_factory = sqlite3.Row
        self._init_tables()

    def _init_tables(self):
        with self.lock:
            cur = self.conn.cursor()
            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS devices (
                    mac TEXT PRIMARY KEY,
                    first_seen REAL,
                    last_seen REAL,
                    last_ip TEXT,
                    last_port INTEGER
                )
                """
            )

            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS logs (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    mac TEXT,
                    ts REAL,
                    level TEXT,
                    message TEXT,
                    FOREIGN KEY(mac) REFERENCES devices(mac)
                )
                """
            )

            cur.execute("CREATE INDEX IF NOT EXISTS ix_logs_mac_ts ON logs(mac, ts)")
            self.conn.commit()

    def upsert_device(self, mac: str, ip: str = None, port: int = None, first_seen: float = None, last_seen: float = None):
        now = time.time()
        if first_seen is None:
            first_seen = now
        if last_seen is None:
            last_seen = now

        with self.lock:
            cur = self.conn.cursor()
            cur.execute("SELECT mac FROM devices WHERE mac = ?", (mac,))
            if cur.fetchone():
                cur.execute(
                    "UPDATE devices SET last_seen = ?, last_ip = ?, last_port = ? WHERE mac = ?",
                    (last_seen, ip, port, mac),
                )
            else:
                cur.execute(
                    "INSERT INTO devices(mac, first_seen, last_seen, last_ip, last_port) VALUES (?, ?, ?, ?, ?)",
                    (mac, first_seen, last_seen, ip, port),
                )
            self.conn.commit()

    def insert_log(self, mac: str, log: Dict):
        ts = log.get("timestamp", time.time())
        level = log.get("level", "INFO")
        message = log.get("message", "")

        with self.lock:
            cur = self.conn.cursor()
            cur.execute("INSERT INTO logs(mac, ts, level, message) VALUES (?, ?, ?, ?)", (mac, ts, level, message))
            self.conn.commit()

            # Trim to last 100 logs per device
            cur.execute("SELECT COUNT(1) as c FROM logs WHERE mac = ?", (mac,))
            row = cur.fetchone()
            count = row["c"] if row else 0
            if count > 100:
                # delete oldest
                to_delete = count - 100
                cur.execute(
                    "DELETE FROM logs WHERE id IN (SELECT id FROM logs WHERE mac = ? ORDER BY ts ASC LIMIT ?)",
                    (mac, to_delete),
                )
                self.conn.commit()

    def get_logs(self, mac: str, limit: int = 100) -> List[Dict]:
        with self.lock:
            cur = self.conn.cursor()
            cur.execute(
                "SELECT ts, level, message FROM (SELECT ts, level, message FROM logs WHERE mac = ? ORDER BY ts DESC LIMIT ?) ORDER BY ts ASC",
                (mac, limit),
            )
            rows = cur.fetchall()
            return [{"timestamp": r["ts"], "level": r["level"], "message": r["message"]} for r in rows]

    def delete_logs(self, mac: str):
        with self.lock:
            cur = self.conn.cursor()
            cur.execute("DELETE FROM logs WHERE mac = ?", (mac,))
            self.conn.commit()

    def delete_device(self, mac: str):
        with self.lock:
            cur = self.conn.cursor()
            cur.execute("DELETE FROM logs WHERE mac = ?", (mac,))
            cur.execute("DELETE FROM devices WHERE mac = ?", (mac,))
            self.conn.commit()

    def get_all_devices(self) -> List[Dict]:
        with self.lock:
            cur = self.conn.cursor()
            cur.execute("SELECT mac, first_seen, last_seen, last_ip, last_port FROM devices ORDER BY last_seen DESC")
            rows = cur.fetchall()
            return [
                {
                    "mac_address": r["mac"],
                    "first_seen": r["first_seen"],
                    "last_seen": r["last_seen"],
                    "last_ip": r["last_ip"],
                    "last_port": r["last_port"],
                }
                for r in rows
            ]

    def total_devices(self) -> int:
        with self.lock:
            cur = self.conn.cursor()
            cur.execute("SELECT COUNT(1) as c FROM devices")
            row = cur.fetchone()
            return int(row["c"] or 0)
