import os

from file import Path


_NS = f"cppbind::c"

MAKE_OWNING_STRUCT = f"{_NS}::make_owning_struct"
MAKE_NON_OWNING_STRUCT = f"{_NS}::make_non_owning_struct"
STRUCT_CAST = f"{_NS}::struct_cast"
NON_OWNING_STRUCT_CAST = f"{_NS}::non_owning_struct_cast"


def make_owning_struct(t, args):
    return f"{MAKE_OWNING_STRUCT}<{t}, {t.without_const().target()}>({args})"


def make_non_owning_struct(t, ptr):
    return f"{MAKE_NON_OWNING_STRUCT}<{t}, {t.without_const().target()}>({ptr})"


def struct_cast(t, what):
    return f"{STRUCT_CAST}<{t}>({what})"

def non_owning_struct_cast(t, what):
    return f"{NON_OWNING_STRUCT_CAST}<{t}>({what})"
