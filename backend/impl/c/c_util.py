import os

from file import Path


_NS = f"cppbind::c"

MAKE_OWNING_STRUCT_MEM = f"{_NS}::make_owning_struct_mem"
MAKE_OWNING_STRUCT_ARGS = f"{_NS}::make_owning_struct_args"
MAKE_NON_OWNING_STRUCT = f"{_NS}::make_non_owning_struct"
STRUCT_CAST = f"{_NS}::struct_cast"


def path_c():
    return Path(os.path.join('cppbind', 'c', 'c_util_c.h'))


def path_cc():
    return Path(os.path.join('cppbind', 'c', 'c_util_cc.h'))


def make_owning_struct_mem(s, t, mem):
    return f"{MAKE_OWNING_STRUCT_MEM}<{t}>({s}, {mem})"


def make_owning_struct_args(s, t, args):
    if args:
        return f"{MAKE_OWNING_STRUCT_ARGS}<{t}>({s}, {args})"
    else:
       return f"{MAKE_OWNING_STRUCT_ARGS}<{t}>({s})"


def make_non_owning_struct(s, ptr):
    return f"{MAKE_NON_OWNING_STRUCT}({s}, {ptr})"


def struct_cast(t, what):
    return f"{STRUCT_CAST}<{t}>({what})"
