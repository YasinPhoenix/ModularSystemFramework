"""
Parse newline-delimited device protocol messages.
Strict, fast, no regex dependency for hot path parsing.
"""

from typing import Optional, Dict


class MessageParser:
    """
    Expected protocol:

    TYPE:IDENTIFY|MAC:XX:XX:XX|NAME:device
    TYPE:LOG|MAC:XX:XX:XX|LEVEL:INFO|MSG:hello
    TYPE:DATA|MAC:XX:XX:XX|KEY:x|VALUE:y
    TYPE:PONG|MAC:XX:XX:XX|TS:123
    TYPE:PING|TS:123   (server -> device, not parsed for registry)
    """

    @staticmethod
    def parse_message(line: str) -> Optional[Dict]:
        if not line:
            return None

        line = line.strip()

        if line.startswith("TYPE:LOG|"):
            return MessageParser._parse_kv(line, "LOG")

        if line.startswith("TYPE:DATA|"):
            return MessageParser._parse_kv(line, "DATA")

        if line.startswith("TYPE:PONG|"):
            return MessageParser._parse_pong(line)

        if line.startswith("TYPE:IDENTIFY|"):
            return MessageParser._parse_identify(line)

        return None

    @staticmethod
    def _parse_kv(line: str, msg_type: str) -> Optional[Dict]:
        parts = line.split("|")
        out = {"type": msg_type}

        for p in parts[1:]:
            if ":" not in p:
                continue
            k, v = p.split(":", 1)
            out[k.lower()] = v

        if "mac" not in out:
            return None

        if msg_type == "LOG":
            return {
                "type": "LOG",
                "mac_address": out.get("mac"),
                "level": out.get("level", "INFO"),
                "message": out.get("msg", ""),
            }

        if msg_type == "DATA":
            return {
                "type": "DATA",
                "mac_address": out.get("mac"),
                "key": out.get("key"),
                "value": out.get("value"),
            }

        return None

    @staticmethod
    def _parse_pong(line: str) -> Optional[Dict]:
        parts = line.split("|")
        mac = None
        ts = None

        for p in parts:
            if p.startswith("MAC:"):
                mac = p.split(":", 1)[1]
            elif p.startswith("TS:"):
                ts = int(p.split(":", 1)[1])

        if not mac:
            return None

        return {
            "type": "PONG",
            "mac_address": mac,
            "timestamp": ts,
        }

    @staticmethod
    def _parse_identify(line: str) -> Optional[Dict]:
        parts = line.split("|")
        mac = None
        name = None

        for p in parts:
            if p.startswith("MAC:"):
                mac = p.split(":", 1)[1]
            elif p.startswith("NAME:"):
                name = p.split(":", 1)[1]

        if not mac:
            return None

        return {
            "type": "IDENTIFY",
            "mac_address": mac,
            "name": name,
        }

    @staticmethod
    def make_ping(timestamp: int) -> str:
        return f"TYPE:PING|TS:{timestamp}\n"

    @staticmethod
    def make_identify_request() -> str:
        return "TYPE:IDENTIFY_REQUEST\n"

    @staticmethod
    def make_disconnect(reason: str) -> str:
        return f"TYPE:DISCONNECT|REASON:{reason}\n"