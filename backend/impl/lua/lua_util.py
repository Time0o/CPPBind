import type_info as ti

from backend import Backend
from text import code


NS = "__lua_util"


def tointegral(t, i):
    return f"{NS}::tointegral<{t}>(L, {i})"


def tofloating(t, i):
    return f"{NS}::tofloating<{t}>(L, {i})"


def topointer(t, i):
    return f"{ti.typed_pointer_cast(t, f'*static_cast<void **>(lua_touserdata(L, {i}))')}"


def pushintegral(arg, constexpr=False):
    if constexpr:
        return f"{NS}_pushintegral_constexpr(L, {arg})"
    else:
        return f"{NS}::pushintegral(L, {arg})"


def pushfloating(arg, constexpr=False):
    if constexpr:
        return f"{NS}_pushfloating_constexpr(L, {arg})"
    else:
        return f"{NS}::pushfloating(L, {arg})"


def pushpointer(arg, owning=False):
    return f"*static_cast<void **>(lua_newuserdata(L, sizeof(void *))) = {ti.make_typed(arg, owning)}"


def setmeta(t):
    return f'{NS}::setmetatable(L, "{t.format(mangled=True)}");'


def define():
    return code(
        """
        #include <cassert>
        #include <limits>
        #include <utility>

        namespace {ns}
        {{

        {ownership}

        {createmetatables}

        {getmetatable}

        {setmetatable}

        {peek_and_push}

        }} // namespace {ns}
        """,
        ns=NS,
        ownership=_ownership(),
        createmetatables=_createmetatables(Backend().records()),
        getmetatable=_getmetatable(),
        setmetatable=_setmetatable(),
        peek_and_push=_peek_and_push())


def _ownership():
    return code(
        f"""
        {ti.TYPED_PTR} *_self(lua_State *L, bool return_self = false)
        {{
          if (lua_gettop(L) != 1) {{
            luaL_error(L, "missing self parameter, did you forget to use ':'?");
            return nullptr;
          }}

          auto userdata = lua_touserdata(L, 1);

          if (return_self)
            lua_pushvalue(L, 1);

          return {ti.get_typed('*static_cast<void **>(userdata)')};
        }}

        int _own(lua_State *L)
        {{
          _self(L, true)->own();
          return 1;
        }}

        int _disown(lua_State *L)
        {{
          _self(L, true)->disown();
          return 1;
        }}

        int _copy(lua_State *L)
        {{
          *static_cast<void **>(lua_newuserdata(L, sizeof(void *))) = _self(L)->copy();

          lua_getmetatable(L, 1);
          lua_setmetatable(L, -2);

          return 1;
        }}

        int _move(lua_State *L)
        {{
          *static_cast<void **>(lua_newuserdata(L, sizeof(void *))) = _self(L)->move();

          lua_getmetatable(L, 1);
          lua_setmetatable(L, -2);

          return 1;
        }}

        int _delete(lua_State *L)
        {{
          delete _self(L);
          return 0;
        }}
        """)


def _createmetatables(records):
    define = []
    call = []

    define.append(_creategenericmetatable())
    call.append(f"createmetatable_generic(L);")

    for r in records:
        if r.is_abstract():
            continue

        define.append(_createmetatable(r))
        call.append(f"createmetatable_{r.name_lua}(L);")

    return code(
        """
        {define}

        void createmetatables(lua_State *L)
        {{
          {call}
        }}
        """,
        define='\n\n'.join(define),
        call='\n'.join(call))


def _creategenericmetatable():
    return code(
        """
        char key_generic;

        void createmetatable_generic(lua_State *L)
        {
          lua_pushlightuserdata(L, static_cast<void *>(&key_generic));

          lua_newtable(L);

          lua_pushcfunction(L, _own);
          lua_setfield(L, -2, "_own");

          lua_pushcfunction(L, _disown);
          lua_setfield(L, -2, "_disown");

          lua_pushcfunction(L, _delete);
          lua_setfield(L, -2, "__gc");

          lua_pushvalue(L, -1);
          lua_setfield(L, -2, "__index");

          lua_settable(L, LUA_REGISTRYINDEX);
        }
        """)


def _createmetatable(r):
    set_methods = []

    for f in r.functions:
        if f.is_constructor() or f.is_destructor():
            continue

        set_methods.append(code(
            f"""
            lua_pushcfunction(L, __{r.name_lua}::{f.name_lua});
            lua_setfield(L, -2, "{f.name_unqualified_lua}");
            """))

    if r.is_copyable():
        set_methods.append(code(
          """
          lua_pushcfunction(L, _copy);
          lua_setfield(L, -2, "_copy");
          """))

    if r.is_moveable():
        set_methods.append(code(
          """
          lua_pushcfunction(L, _move);
          lua_setfield(L, -2, "_move");
          """))

    return code(
        f"""
        void createmetatable_{r.name_lua}(lua_State *L)
        {{{{
          lua_pushstring(L, "{r.type.format(mangled=True)}");

          lua_newtable(L);

          {{set_methods}}

          lua_pushcfunction(L, _own);
          lua_setfield(L, -2, "_own");

          lua_pushcfunction(L, _disown);
          lua_setfield(L, -2, "_disown");

          lua_pushcfunction(L, _delete);
          lua_setfield(L, -2, "__gc");

          lua_pushvalue(L, -1);
          lua_setfield(L, -2, "__index");

          lua_settable(L, LUA_REGISTRYINDEX);
        }}}}
        """,
        set_methods='\n\n'.join(set_methods))


def _getmetatable():
    return code(
        f"""
        void getmetatable(lua_State *L, char const *key)
        {{
          static bool created = false;

          if (!created) {{
            createmetatables(L);
            created = true;
          }}

          lua_pushstring(L, key);
          lua_gettable(L, LUA_REGISTRYINDEX);

          if (lua_istable(L, -1))
            return;

          lua_pop(L, 1);

          lua_pushlightuserdata(L, static_cast<void *>(&key_generic));
          lua_gettable(L, LUA_REGISTRYINDEX);

          assert(lua_istable(L, -1));
        }}
        """)

def _setmetatable():
    return code(
        """
        void setmetatable(lua_State *L, char const *key)
        {
          getmetatable(L, key);
          lua_setmetatable(L, -2);
        }
        """)

def _peek_and_push():
    return code(
        f"""
        template<typename T>
        T tointegral(lua_State *L, int arg)
        {{
          int valid = 0;
          auto i = lua_tointegerx(L, arg, &valid);

          luaL_argcheck(L, valid, arg,
                        "not convertible to lua_Integer");
          luaL_argcheck(L, std::numeric_limits<T>::is_signed || i >= 0, arg,
                        "should be non-negative");
          luaL_argcheck(L, i >= std::numeric_limits<T>::min(), arg,
                        "conversion would underflow");
          luaL_argcheck(L, i <= std::numeric_limits<T>::max(), arg,
                        "conversion would overflow");

          return static_cast<T>(i);
        }}

        template<typename T>
        T tofloating(lua_State *L, int arg)
        {{
          int valid = 0;
          auto f = lua_tonumberx(L, arg, &valid);

          luaL_argcheck(L, valid, arg,
                        "not convertible to lua_Number");
          luaL_argcheck(L, f >= std::numeric_limits<T>::lowest(), arg,
                        "conversion would underflow");
          luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                        "conversion would overflow");

          return static_cast<T>(f);
        }}

        template<typename T>
        void pushintegral(lua_State *L, T val)
        {{
          if (val < std::numeric_limits<lua_Integer>::min() ||
              val > std::numeric_limits<lua_Integer>::max())
            luaL_error(L, "result not representable by lua_Integer");

          lua_pushinteger(L, val);
        }}

        template<typename T>
        void pushfloating(lua_State *L, T val)
        {{
          if (val < std::numeric_limits<lua_Number>::lowest() ||
              val > std::numeric_limits<lua_Number>::max())
            luaL_error(L, "result not representable by lua_Number");

          lua_pushnumber(L, val);
        }}

        #define {NS}_pushintegral_constexpr(L, VAL) \\
          static_assert(VAL >= std::numeric_limits<lua_Integer>::min() && \\
                        VAL <= std::numeric_limits<lua_Integer>::max(), \\
                        "parameter not representable by lua_Integer"); \\
          lua_pushinteger(L, VAL);


        #define {NS}_pushfloating_constexpr(L, VAL) \\
          static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \\
                        VAL <= std::numeric_limits<lua_Number>::max(), \\
                        "parameter not representable by lua_Number"); \\
          lua_pushnumber(L, VAL);
        """)
