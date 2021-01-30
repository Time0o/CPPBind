from backend import *
from cppbind import Identifier

import text


class LuaBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        input_file = self.input_file()

        self._wrapper_module = self.output_file(
            input_file.modified(filename='{filename}_lua', ext='.cpp'))

    def wrap_before(self):
        self._wrapper_module.append(text.code(
            """
            #define LUA_LIB

            #include <limits>
            #include <utility>

            extern "C"
            {{
              #include "lua.h"
              #include "lauxlib.h"
            }}

            {input_include}

            namespace
            {{

            {util}
            """,
            input_include=text.include(self.input_file().basename()),
            util=self._util()
        ))

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(text.code(
            """
            {module_create}

            {module_properties}

            }} // namespace

            extern "C"
            {{

            LUALIB_API int luaopen_{module}(lua_State *L)
            {{
              {module}_create(L);
              {module}_properties(L);
              return 1;
            }}

            }} // extern "C"
            """,
            module=self._module(),
            module_create=self._module_create(),
            module_properties=self._module_properties()
        ))

    def wrap_variable(self, c):
        pass

    def wrap_function(self, f):
        self._wrapper_module.append(text.code(
            """
            int {function_name}(lua_State *L)
            {{
              {check_num_parameters}

              {declare_parameters}

              {peek_parameters}

              {forward_function}

              {return_references}

              return {num_returns};
            }}
            """,
            function_name=self._lua_function_name(f),
            check_num_parameters=self._function_check_num_parameters(f),
            declare_parameters=self._function_declare_parameters(f),
            peek_parameters=self._function_peek_parameters(f),
            forward_function=self._function_forward(f),
            return_references=self._function_return_references(f),
            num_returns=self._function_num_returns(f)
        ))

    @staticmethod
    def _util():
        return text.code(
            """
            template<typename T>
            T _lua_tointegral(lua_State *L, int arg)
            {
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
            }

            template<typename T>
            T _lua_tofloating(lua_State *L, int arg)
            {
              int valid = 0;
              auto f = lua_tonumberx(L, arg, &valid);

              luaL_argcheck(L, valid, arg,
                            "not convertible to lua_Number");
              luaL_argcheck(L, f >= std::numeric_limits<T>::lowest(), arg,
                            "conversion would underflow");
              luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                            "conversion would overflow");

              return static_cast<T>(f);
            }

            #define _lua_pushintegral_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Integer>::min() && \\
                            VAL <= std::numeric_limits<lua_Integer>::max(), \\
                            "parameter not representable by lua_Integer"); \\
              lua_pushinteger(L, VAL);

            template<typename T>
            void _lua_pushintegral(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Integer>::min() ||
                  val > std::numeric_limits<lua_Integer>::max())
                luaL_error(L, "result not representable by lua_Integer");

              lua_pushinteger(L, val);
            }

            #define _lua_pushfloating_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \\
                            VAL <= std::numeric_limits<lua_Number>::max(), \\
                            "parameter not representable by lua_Number"); \\
              lua_pushnumber(L, VAL);

            template<typename T>
            void _lua_pushfloating(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Number>::lowest() ||
                  val > std::numeric_limits<lua_Number>::max())
                luaL_error(L, "result not representable by lua_Number");

              lua_pushnumber(L, val);
            }
            """)

    @staticmethod
    def _function_check_num_parameters(f):
        num_min = len([p for p in f.parameters if not p.default_argument])
        num_max = len(f.parameters)

        if num_min == num_max:
            return text.code(
                f"""
                if (lua_gettop(L) != {num_min})
                  return luaL_error(L, "function expects {num_min} arguments");
                """)

        return text.code(
            f"""
            if (lua_gettop(L) < {num_min} || lua_gettop(L) > {num_max})
              return luaL_error(L, "function expects between {num_min} and {num_max} arguments");
            """)

    @staticmethod
    def _function_declare_parameters(f):
        if len(f.parameters) == 0:
            return

        declarations = []

        def declare(p):
            t = p.type

            if t.is_reference():
                t = t.referenced()

            if t.is_const():
                t = t.without_const()

            declaration = f"{t} {p.name}"

            if p.default_argument is not None:
                if t.is_scoped_enum():
                    default = f"static_cast<{t}>({p.default_argument})"
                else:
                    default = f"{p.default_argument}"

                declaration = f"{declaration} = {default}"

            return f"{declaration};"

        return '\n'.join(declare(p) for p in f.parameters)

    @classmethod
    def _function_peek_parameters(cls, f):
        if len(f.parameters) == 0:
            return

        return '\n'.join(cls._function_peek_parameter(p, i)
                         for i, p in enumerate(f.parameters, start=1))

    @classmethod
    def _function_peek_parameter(cls, p, i):
        t = p.type

        if t.is_reference():
            t = t.referenced()

        # check
        check_type = f"luaL_checktype(L, {i}, {cls._lua_type_encoding(t)})"

        # peek
        to_type = cls._lua_peek(t, i)

        # cast
        cast_type = None

        if t.is_pointer():
            cast_type = f"static_cast<{t}>"

        if cast_type is not None:
            to_type = f"{cast_type}({to_type})"

        if p.default_argument is not None:
            # optional
            peek = text.code(
                f"""
                if (lua_gettop(L) >= {i}) {{
                  {check_type};
                  {p.name} = {to_type};
                }}
                """)
        else:
            peek = text.code(
                f"""
                {check_type};
                {p.name} = {to_type};
                """)

        return peek

    @classmethod
    def _function_forward(cls, f):
        def forward_parameter(p):
            if p.type.is_rvalue_reference():
                return f"std::move({p.name})"
            else:
                return f"{p.name}"

        forward_parameters = ', '.join(forward_parameter(p) for p in f.parameters)

        forward = f"{f.name}({forward_parameters})"

        if not f.return_type.is_void():
            t = f.return_type

            if t.is_lvalue_reference():
                t = t.referenced()

            forward = cls._lua_push(t, forward)

        return f"{forward};"

    @classmethod
    def _function_return_references(cls, f):
        ref_returns = []

        for p in cls._function_non_const_lvalue_reference_parameters(f):
            ref_returns.append(cls._lua_push(p.type.referenced(), p.name))

        return '\n'.join(f"{ret};" for ret in ref_returns)

    @classmethod
    def _function_num_returns(cls, f):
        num_regular_returns = int(not f.return_type.is_void())
        num_reference_returns = len(cls._function_non_const_lvalue_reference_parameters(f))

        return num_regular_returns + num_reference_returns

    @staticmethod
    def _function_non_const_lvalue_reference_parameters(f):
        ref_parameters = []

        for p in f.parameters:
            t = p.type

            if not t.is_lvalue_reference():
                continue

            t = t.referenced()

            if t.is_const():
                continue

            ref_parameters.append(p)

        return ref_parameters

    def _module(self):
        return self.input_file().filename()

    def _module_create(self):
        if not self.functions():
            return text.code(
                """
                void {module}_create(lua_State *L)
                {{ lua_newtable(L); }}
                """,
                module=self._module())
        else:
            module_function_table = []

            for f in self.functions():
                entry = self._lua_function_name(f)

                module_function_table.append(f'{{"{entry}", {entry}}}')

            return text.code(
                """
                luaL_Reg lib[] = {{
                  {module_function_table}
                }};

                void {module}_create(lua_State *L)
                {{ luaL_newlib(L, lib); }}
                """,
                module=self._module(),
                module_function_table=',\n'.join(module_function_table))

    def _module_properties(self):
        module_properties = []

        for v in self.variables():
            lua_name = self._lua_variable_name(v)
            lua_type = self._lua_type(v.type)

            module_properties.append(text.code(
                """
                lua_pushstring(L, "{lua_name}");
                {lua_push};
                lua_settable(L, -3);
                """,
                lua_name=self._lua_variable_name(v),
                lua_push=self._lua_push(v.type, v.name, constexpr=True),
                end=''))

        return text.code(
            """
            void {module}_properties(lua_State *L)
            {{
              {module_properties}
            }}
            """,
            module=self._module(),
            module_properties='\n\n'.join(module_properties))

    @classmethod
    def _lua_peek(cls, t, i):
        if t.is_scoped_enum():
            it = t.without_enum()
            return f"static_cast<{t}>(_lua_tointegral<{it}>(L, {i}))"
        elif t.is_integral():
            return f"_lua_tointegral<{t}>(L, {i})"
        elif t.is_floating():
            return f"_lua_tofloating<{t}>(L, {i})"
        else:
            lua_type = cls._lua_type(t)
            return f"lua_to{lua_type}(L, {i})"

    @classmethod
    def _lua_push(cls, t, v, constexpr=False):
        if t.is_scoped_enum():
            it = t.without_enum()
            return f"_lua_pushintegral<{it}>(L, static_cast<{it}>({v}))"
        elif t.is_integral():
            if constexpr:
                return f"_lua_pushintegral_constexpr(L, {v})"
            else:
                return f"_lua_pushintegral<{t}>(L, {v})"
        elif t.is_floating():
            if constexpr:
                return f"_lua_pushfloating_constexpr(L, {v})"
            else:
                return f"_lua_pushfloating<{t}>(L, {v})"
        elif t.is_pointer():
            return f"lua_pushlightuserdata(L, {v})"
        else:
            lua_type = cls._lua_type(t)
            return f"lua_push{lua_type}(L, {v})"

    @staticmethod
    def _lua_type(t):
        if t.is_integral() or t.is_enum():
            return 'integer'
        elif t.is_floating():
            return 'number'
        elif t.is_boolean():
            return 'boolean'
        elif t.is_pointer():
            return 'userdata'
        else:
            raise ValueError('TODO')

    @staticmethod
    def _lua_type_encoding(t):
        if t.is_integral() or t.is_floating() or t.is_enum():
            return 'LUA_TNUMBER'
        elif t.is_boolean():
            return 'LUA_TBOOLEAN'
        elif t.is_pointer():
            return 'LUA_TLIGHTUSERDATA'
        else:
            raise ValueError('TODO')

    @staticmethod
    def _lua_variable_name(v):
        return v.name.format(case=Identifier.SNAKE_CASE_CAP_ALL,
                             quals=Identifier.REPLACE_QUALS)

    @staticmethod
    def _lua_function_name(f):
        return f.name_overloaded.format(case=Identifier.SNAKE_CASE,
                                        quals=Identifier.REPLACE_QUALS)

    @staticmethod
    def _lua_parameter_name(p):
        return p.name.format(case=Identifier.SNAKE_CASE)
