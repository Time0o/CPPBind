import type_info as ti
from backend import Backend
from text import code


_NS = "cppbind::lua"

TOINTEGRAL = f"{_NS}::tointegral"
TOFLOATING = f"{_NS}::tofloating"
PUSHINTEGRAL = f"{_NS}::pushintegral"
PUSHINTEGRAL_CONSTEXPR = f"cppbind_lua_pushintegral_constexpr"
PUSHFLOATING = f"{_NS}::pushfloating"
PUSHFLOATING_CONSTEXPR = f"cppbind_lua_pushfloating_constexpr"
CREATEMETATABLE = f"{_NS}::createmetatable"
CREATEMETATABLE_GENERIC = f"{_NS}::createmetatable_generic"
SETMETATABLE = f"{_NS}::setmetatable"


def include():
    return '#include "cppbind/lua/lua_util.hpp"'


def createmetatables():
    create = [f"{CREATEMETATABLE_GENERIC}(L);"]

    for r in Backend().records():
        if r.is_abstract():
            continue

        create.append(_createmetatable(r))

    return code(
        """
        namespace {ns} {{
          void createmetatables(lua_State *L)
          {{
            {create}
          }}
        }}
        """,
        ns=_NS,
        create='\n\n'.join(create))


def _createmetatable(r):
    function_entries = []

    for f in r.functions():
        if f.is_constructor() or f.is_destructor():
            continue

        entry_name = f'"{f.name_target(qualified=False)}"'
        entry = f"__{r.name_target()}::{f.name_target()}"

        function_entries.append(f"{{{entry_name}, {entry}}}")

    if r.is_copyable():
        function_entries.append('{"_copy", _copy}')

    if r.is_moveable():
        function_entries.append('{"_move", _move}')

    key = f'"METATABLE_{r.type().mangled()}"'

    return code(
        """
        createmetatable(L, {key},
          {{
            {function_entries}
          }});
        """,
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
    return f'{SETMETATABLE}(L, "METATABLE_{t.mangled()}");'
