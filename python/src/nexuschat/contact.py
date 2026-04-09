"""Contact object."""

from datetime import date, datetime, timezone
from typing import Optional

from . import const, props
from .capi import ffi, lib
from .chat import Chat
from .cutil import from_nc_charpointer, from_optional_nc_charpointer


class Contact:
    """Nexus-Chat Contact.

    You obtain instances of it through :class:`deltachat.account.Account`.
    """

    def __init__(self, account, id) -> None:
        from .account import Account

        assert isinstance(account, Account), repr(account)
        self.account = account
        self.id = id

    def __eq__(self, other) -> bool:
        if other is None:
            return False
        return self.account._nc_context == other.account._nc_context and self.id == other.id

    def __ne__(self, other) -> bool:
        return not self == other

    def __repr__(self) -> str:
        return f"<Contact id={self.id} addr={self.addr} nc_context={self.account._nc_context}>"

    @property
    def _nc_contact(self):
        return ffi.gc(lib.nc_get_contact(self.account._nc_context, self.id), lib.nc_contact_unref)

    @props.with_doc
    def addr(self) -> str:
        """normalized e-mail address for this account."""
        return from_nc_charpointer(lib.nc_contact_get_addr(self._nc_contact))

    @props.with_doc
    def name(self) -> str:
        """display name for this contact."""
        return from_nc_charpointer(lib.nc_contact_get_display_name(self._nc_contact))

    # deprecated alias
    display_name = name

    @props.with_doc
    def last_seen(self) -> date:
        """Last seen timestamp."""
        return datetime.fromtimestamp(lib.nc_contact_get_last_seen(self._nc_contact), timezone.utc)

    def is_blocked(self):
        """Return True if the contact is blocked."""
        return lib.nc_contact_is_blocked(self._nc_contact)

    def set_blocked(self, block=True):
        """[Deprecated, use block/unblock methods] Block or unblock a contact."""
        return lib.nc_block_contact(self.account._nc_context, self.id, block)

    def block(self):
        """Block this contact. Message will not be seen/retrieved from this contact."""
        return lib.nc_block_contact(self.account._nc_context, self.id, True)

    def unblock(self):
        """Unblock this contact. Messages from this contact will be retrieved (again)."""
        return lib.nc_block_contact(self.account._nc_context, self.id, False)

    def is_verified(self) -> bool:
        """Return True if the contact is verified."""
        return lib.nc_contact_is_verified(self._nc_contact) == 2

    def get_verifier(self, contact) -> Optional["Contact"]:
        """Return the address of the contact that verified the contact."""
        verifier_id = lib.nc_contact_get_verifier_id(contact._nc_contact)
        if verifier_id == 0:
            return None
        return Contact(self.account, verifier_id)

    def get_profile_image(self) -> Optional[str]:
        """Get contact profile image.

        :returns: path to profile image, None if no profile image exists.
        """
        nc_res = lib.nc_contact_get_profile_image(self._nc_contact)
        return from_optional_nc_charpointer(nc_res)

    def make_vcard(self) -> str:
        """Make a contact vCard.

        :returns: vCard
        """
        nc_context = self.account._nc_context
        return from_nc_charpointer(lib.nc_make_vcard(nc_context, self.id))

    @property
    def status(self):
        """Get contact status.

        :returns: contact status, empty string if it doesn't exist.
        """
        return from_nc_charpointer(lib.nc_contact_get_status(self._nc_contact))

    def create_chat(self):
        """create or get an existing 1:1 chat object for the specified contact or contact id.

        :param contact: chat_id (int) or contact object.
        :returns: a :class:`nexuschat.chat.Chat` object.
        """
        nc_context = self.account._nc_context
        chat_id = lib.nc_create_chat_by_contact_id(nc_context, self.id)
        assert chat_id > const.NC_CHAT_ID_LAST_SPECIAL, chat_id
        return Chat(self.account, chat_id)

    # deprecated name
    get_chat = create_chat
