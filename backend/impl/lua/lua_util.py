import os
import type_info as ti
from backend import Backend
from cppbind import Identifier as Id
from file import Path
from text import code


_NS = f"cppbind::lua"

BIND_OWN = f"{_NS}::bind_own"
BIND_DISOWN = f"{_NS}::bind_disown"
BIND_COPY = f"{_NS}::bind_copy"
BIND_MOVE = f"{_NS}::bind_move"
BIND_DELETE = f"{_NS}::bind_delete"
CREATEMETATABLE = f"{_NS}::createmetatable"
CREATEMETATABLE_GENERIC = f"{_NS}::createmetatable_generic"
GETMETATABLE = f"{_NS}::getmetatable"
SETMETATABLE = f"{_NS}::setmetatable"
TOINTEGRAL = f"{_NS}::tointegral"
TOFLOATING = f"{_NS}::tofloating"
PUSHINTEGRAL = f"{_NS}::pushintegral"
PUSHINTEGRAL_CONSTEXPR = f"cppbind_lua_pushintegral_constexpr"
PUSHFLOATING = f"{_NS}::pushfloating"
PUSHFLOATING_CONSTEXPR = f"cppbind_lua_pushfloating_constexpr"


def path():
    return Path(os.path.join('cppbind', 'lua', 'lua_util.h'))


def createmetatable(r):
    function_entries = []

    for f in r.functions():
        if f.is_constructor() or f.is_destructor():
            continue

        entry_name = f.name_target(quals=Id.REMOVE_QUALS)
        entry = f.name_target()

        function_entries.append(f'{{"{entry_name}", {entry}}}')

    if r.is_copyable():
        function_entries.append(f'{{"copy", {BIND_COPY}}}')

    if r.is_moveable():
        function_entries.append(f'{{"move", {BIND_MOVE}}}')

    key = f'"METATABLE_{r.type().mangled()}"'

    if not function_entries:
        return f"{CREATEMETATABLE}(L, {key}, {{}});"

    return code(
        """
        {createmetatable}(L, {key},
          {{
            {function_entries}
          }});
        """,
        createmetatable=CREATEMETATABLE,
        key=key,
        function_entries=',\n'.join(function_entries))


def tointegral(t, i):
    return f"{TOINTEGRAL}<{t}>(L, {i})"


def tofloating(t, i):
    return f"{TOFLOATING}<{t}>(L, {i})"


def topointer(t, i):
    return f"{ti.typed_pointer_cast(t, f'*static_cast<void **>(lua_touserdata(L, {i}))')}"


def pushintegral(arg, constexpr=False):
    if constexpr:
        return f"{PUSHINTEGRAL_CONSTEXPR}(L, {arg})"
    else:
        return f"{PUSHINTEGRAL}(L, {arg})"


def pushfloating(arg, constexpr=False):
    if constexpr:
        return f"{PUSHFLOATING_CONSTEXPR}(L, {arg})"
    else:
        return f"{PUSHFLOATING}(L, {arg})"


def pushpointer(arg, owning=False):
    return f"*static_cast<void **>(lua_newuserdata(L, sizeof(void *))) = {ti.make_typed(arg, owning)}"


def setmetatable(t):
    return f'{SETMETATABLE}(L, "METATABLE_{t.without_const().mangled()}");'
