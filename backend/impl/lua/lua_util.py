from text import code
from type_info import TypeInfo as TI


class LuaUtil:
    NS = "__lua_util"

    def __init__(self, wrapper):
        self._records = wrapper.records()

    @classmethod
    def tointegral(cls, t, i):
        return f"{cls.NS}::tointegral<{t}>(L, {i})"

    @classmethod
    def tofloating(cls, t, i):
        return f"{cls.NS}::tofloating<{t}>(L, {i})"

    @classmethod
    def pushintegral(cls, arg, constexpr=False):
        if constexpr:
            return f"{cls.NS}_pushintegral_constexpr(L, {arg});"
        else:
            return f"{cls.NS}::pushintegral(L, {arg});"

    @classmethod
    def pushfloating(cls, arg, constexpr=False):
        if constexpr:
            return f"{cls.NS}_pushfloating_constexpr(L, {arg});"
        else:
            return f"{cls.NS}::pushfloating(L, {arg});"

    @classmethod
    def pushpointer(cls, t, arg, owning=False):
        push = f"new (lua_newuserdata(L, sizeof({TI.TYPED_PTR}))) {TI.TYPED_PTR}({arg});"

        if owning:
            push = code(
                f"""
                {{push}}

                {cls.NS}::get_record_metatable(L, "{t.format(mangled=True)}");
                lua_setmetatable(L, -2);
                """,
                push=push)

        return push

    def code(self):
        return code(
            """
            #include <cassert>
            #include <limits>
            #include <utility>

            namespace {ns}
            {{

            {create_record_metatables}

            void get_record_metatable(lua_State *L, char const *__registry_key)
            {{
              static bool created = false;

              if (!created)
                create_record_metatables(L);

              lua_pushstring(L, __registry_key);
              lua_gettable(L, LUA_REGISTRYINDEX);

              assert(lua_istable(L, -1));
            }}

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

            }} // namespace {ns}

            #define {ns}_pushintegral_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Integer>::min() && \\
                            VAL <= std::numeric_limits<lua_Integer>::max(), \\
                            "parameter not representable by lua_Integer"); \\
              lua_pushinteger(L, VAL);


            #define {ns}_pushfloating_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \\
                            VAL <= std::numeric_limits<lua_Number>::max(), \\
                            "parameter not representable by lua_Number"); \\
              lua_pushnumber(L, VAL);
            """,
            ns=self.NS,
            create_record_metatables=self._create_record_metatables())

    def _create_record_metatables(self):
        define = []
        call = []

        for r in self._records:
            if r.is_abstract():
                continue

            define.append(self._create_record_metatable(r))
            call.append(f"create_{r.name_lua}_metatable(L);")

        return code(
            """
            {define}

            void create_record_metatables(lua_State *L)
            {{
              {call}
            }}
            """,
            define='\n\n'.join(define),
            call='\n'.join(call))

    @staticmethod
    def _create_record_metatable(r):
        set_methods = []

        for f in r.functions:
            if f.is_constructor() or f.is_destructor():
                continue

            set_methods.append(code(
                f"""
                lua_pushcfunction(L, __{r.name_lua}::{f.name_lua});
                lua_setfield(L, -2, "{f.name_unqualified_lua}");
                """))

        return code(
            f"""
            void create_{r.name_lua}_metatable(lua_State *L)
            {{{{
              lua_pushstring(L, "{r.type.format(mangled=True)}");

              lua_newtable(L);

              {{set_methods}}

              lua_pushcfunction(L, __{r.name_lua}::{r.destructor.name_lua});
              lua_setfield(L, -2, "__gc");

              lua_pushvalue(L, -1);
              lua_setfield(L, -2, "__index");

              lua_settable(L, LUA_REGISTRYINDEX);
            }}}}
            """,
            set_methods='\n\n'.join(set_methods))
