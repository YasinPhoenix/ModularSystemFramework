"""Parse newline-delimited messages from devices."""

import re
from typing import Dict, Optional, Tuple


class MessageParser:
    """Parse newline-delimited text protocol messages."""

    # Regex patterns for parsing
    PATTERN_LOG = re.compile(r"TYPE:LOG\|MAC:([A-F0-9:]+)\|LEVEL:(\w+)\|MSG:(.+)")
    PATTERN_DATA = re.compile(r"TYPE:DATA\|MAC:([A-F0-9:]+)\|KEY:(\w+)\|VALUE:(.+)")
    PATTERN_PONG = re.compile(r"TYPE:PONG\|MAC:([A-F0-9:]+)\|TS:(\d+)")
    PATTERN_IDENTIFY = re.compile(r"TYPE:IDENTIFY\|MAC:([A-F0-9:]+)\|NAME:(.+)")

    @staticmethod
    def parse_message(line: str) -> Optional[Dict]:
        """
        Parse a message line.
        Returns a dict with parsed data or None if invalid format.
        """
        line = line.strip()
        if not line:
            return None

        # Try LOG format
        match = MessageParser.PATTERN_LOG.match(line)
        if match:
            return {
                "type": "LOG",
                "mac_address": match.group(1),
                "level": match.group(2),
                "message": match.group(3),
            }

        # Try DATA format
        match = MessageParser.PATTERN_DATA.match(line)
        if match:
            return {
                "type": "DATA",
                "mac_address": match.group(1),
                "key": match.group(2),
                "value": match.group(3),
            }

        # Try PONG format
        match = MessageParser.PATTERN_PONG.match(line)
        if match:
            return {
                "type": "PONG",
                "mac_address": match.group(1),
                "timestamp": int(match.group(2)),
            }

        # Try IDENTIFY format
        match = MessageParser.PATTERN_IDENTIFY.match(line)
        if match:
            return {
                "type": "IDENTIFY",
                "mac_address": match.group(1),
                "name": match.group(2),
            }

        return None

    @staticmethod
    def make_ping(timestamp: int) -> str:
        """Create a PING message."""
        return f"TYPE:PING|TS:{timestamp}\n"

    @staticmethod
    def make_identify_request() -> str:
        """Create an IDENTIFY request message."""
        return "TYPE:IDENTIFY_REQUEST\n"

    @staticmethod
    def make_disconnect(reason: str) -> str:
        """Create a DISCONNECT message."""
        return f"TYPE:DISCONNECT|REASON:{reason}\n"
