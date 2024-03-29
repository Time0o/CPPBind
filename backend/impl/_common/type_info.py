# Python convenience functions for type_info.h.

from backend import backend
from collections import OrderedDict
from file import Path
from itertools import chain
from text import code
import os


_NS = f"cppbind::type_info"

TYPE = f"{_NS}::type"
TYPE_INSTANCE = f"{_NS}::type_instance"
TYPED_PTR = f"{_NS}::typed_ptr"
MAKE_TYPED = f"{_NS}::make_typed"
GET_TYPED = f"{_NS}::get_typed"
TYPED_POINTER_CASE = f"{_NS}::typed_pointer_cast"
OWN = f"{_NS}::_own"
DISOWN = f"{_NS}::_disown"
COPY = f"{_NS}::_copy"
MOVE = f"{_NS}::_move"
DELETE = f"{_NS}::_delete"


def path():
    return Path(os.path.join('cppbind', 'type_info.h'))


# Create 'type_instance' instantiations for all record and reference types used
# by wrapper functions in the current translation unit.
def type_instances():
    be_records = backend().records(include_declarations=True)

    be_types = backend().types()

    types = OrderedDict()

    def add_type(t, t_bases=None):
        if t.is_void():
            return

        t = t.canonical().unqualified()

        if t_bases is not None:
            t_bases = tuple(t_bases)

        t_key = str(t)

        if t_key in types:
            return

        types[t_key] = (t.mangled(), t, t_bases)

    for r in be_records:
        t = r.type()

        if t in be_types:
            add_type(t, t.base_types(recursive=True))

    for t in be_types:
        if t.is_record():
            add_type(t)
        elif t.is_pointer() or t.is_reference():
            add_type(t.pointee())

    tis = []
    for t_mangled, t, t_bases in types.values():
        template_params = [t]

        if t_bases is not None:
            template_params += t_bases

        template_params = ', '.join(map(str, template_params))

        tis.append(f"type_instance<{template_params}> {t_mangled};")

    if not tis:
        return

    return code(
        """
        namespace {ns} {{
          {tis}
        }}
        """,
        ns=_NS,
        tis='\n'.join(tis))


def make_typed(arg, mem=None, owning=False):
    mem = mem if mem is not None else 'nullptr'
    owning = 'true' if owning else 'false'

    return f"{MAKE_TYPED}({arg}, {mem}, {owning})"


def get_typed(arg):
    return f"{GET_TYPED}({arg})"


def typed_pointer_cast(t, arg):
    return f"{TYPED_POINTER_CASE}<{t}>({arg})"


def own(arg):
    return f"{OWN}({arg})"


def disown(arg):
    return f"{DISOWN}({arg})"


def copy(arg):
    return f"{COPY}({arg})"


def move(arg):
    return f"{MOVE}({arg})"


def delete(arg):
    return f"{DELETE}({arg})"
