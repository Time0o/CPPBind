import os

from file import Path


_NS = f"cppbind::c"

INIT_OWNING_STRUCT = f"{_NS}::init_owning_struct"
INIT_NON_OWNING_STRUCT = f"{_NS}::init_non_owning_struct"
STRUCT_CAST = f"{_NS}::struct_cast"


def path_c():
    return Path(os.path.join('cppbind', 'c', 'c_util_c.h'))


def path_cc():
    return Path(os.path.join('cppbind', 'c', 'c_util_cc.h'))


def init_owning_struct(s, t, args):
    if args:
        return f"{INIT_OWNING_STRUCT}<{t}>({s}, {args})"
    else:
       return f"{INIT_OWNING_STRUCT}<{t}>({s})"


def init_non_owning_struct(s, ptr):
    return f"{INIT_NON_OWNING_STRUCT}({s}, {ptr})"


def struct_cast(t, what):
    return f"{STRUCT_CAST}<{t}>({what})"
