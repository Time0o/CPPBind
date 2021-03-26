import os

from file import Path


_NS = f"cppbind::c"

STRUCT_CAST = f"{_NS}::struct_cast"


def path_c():
    return Path(os.path.join('cppbind', 'c', 'c_util_c.h'))


def path_cc():
    return Path(os.path.join('cppbind', 'c', 'c_util_cc.h'))


def struct_cast(t, what):
    return f"{STRUCT_CAST}<{t}>({what})"
