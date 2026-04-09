from datetime import datetime, timezone
from typing import Callable, Generator, Optional, TypeVar

from .capi import ffi, lib

T = TypeVar("T")


def as_nc_charpointer(obj):
    if obj == ffi.NULL or obj is None:
        return ffi.NULL
    if not isinstance(obj, bytes):
        return obj.encode("utf8")
    return obj


def iter_array(nc_array_t, constructor: Callable[[int], T]) -> Generator[T, None, None]:
    for i in range(lib.nc_array_get_cnt(nc_array_t)):
        yield constructor(lib.nc_array_get_id(nc_array_t, i))


def from_nc_charpointer(obj) -> str:
    if obj != ffi.NULL:
        return ffi.string(ffi.gc(obj, lib.nc_str_unref)).decode("utf8")
    raise ValueError


def from_optional_nc_charpointer(obj) -> Optional[str]:
    if obj != ffi.NULL:
        return ffi.string(ffi.gc(obj, lib.nc_str_unref)).decode("utf8")
    return None


class NCLot:
    def __init__(self, nc_lot) -> None:
        self._nc_lot = nc_lot

    def id(self) -> int:
        return lib.nc_lot_get_id(self._nc_lot)

    def state(self):
        return lib.nc_lot_get_state(self._nc_lot)

    def text1(self):
        return from_nc_charpointer(lib.nc_lot_get_text1(self._nc_lot))

    def text1_meaning(self):
        return lib.nc_lot_get_text1_meaning(self._nc_lot)

    def text2(self):
        return from_nc_charpointer(lib.nc_lot_get_text2(self._nc_lot))

    def timestamp(self):
        ts = lib.nc_lot_get_timestamp(self._nc_lot)
        if ts == 0:
            return None
        return datetime.fromtimestamp(ts, timezone.utc)
