import os

from backend import backend
from collections import OrderedDict
from file import Path
from itertools import chain
from text import code


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


def type_instances():
    be_records = backend().records(include_incomplete=True,
                                   include_abstract=True)

    be_types = backend().types(as_set=True)

    types = OrderedDict()

    def add_type(t, t_bases=None):
        if t.is_void():
            return

        t = t.canonical().unqualified()

        if t_bases is not None:
            t_bases = tuple(t_bases)

        types[str(t)] = (t.mangled(), t, t_bases)

    for r in be_records:
        t = r.type()

        if t in be_types:
            add_type(t, t.base_types())

    for t in be_types:
        if t.is_record():
            add_type(t)
        elif t.is_pointer() or t.is_reference():
            add_type(t.pointee())

        t_helper = t.helper()
        if t_helper:
            types[t_helper] = (f"HELPER_{t.mangled()}", t_helper, None)

    tis = []
    for t_mangled, t, t_bases in types.values():
        template_params = [t]

        if t_bases is not None:
            template_params += t_bases

        template_params = ', '.join(map(str, template_params))

        tis.append(f"type_instance<{template_params}> {t_mangled};")

    return code(
        """
        namespace {ns} {{
          {tis}
        }}
        """,
        ns=_NS,
        tis='\n'.join(tis))


def make_typed(arg, owning=False):
    owning = 'true' if owning else 'false'

    return f"{MAKE_TYPED}({arg}, {owning})"


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
