"""Nexus Chat JSON-RPC high-level API."""

from ._utils import AttrDict, run_bot_cli, run_client_cli
from .account import Account
from .chat import Chat
from .client import Bot, Client
from .const import EventType, SpecialContactId
from .contact import Contact
from .nexuschat import NexusChat
from .message import Message
from .rpc import JsonRpcError, Rpc

__all__ = [
    "Account",
    "AttrDict",
    "Bot",
    "Chat",
    "Client",
    "Contact",
    "NexusChat",
    "EventType",
    "JsonRpcError",
    "Message",
    "SpecialContactId",
    "Rpc",
    "run_bot_cli",
    "run_client_cli",
]
