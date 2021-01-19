from backend import *

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

            extern "C" {{
              #include "lua.h"
              #include "lauxlib.h"
            }}

            {input_include}

            namespace {{

            {wrapper_utils}
            """,
            input_include=text.include(self.input_file().basename()),
            wrapper_utils=self._wrapper_utils()
        ))

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(text.code(
            """
            {module_create}

            {module_properties}

            }} // namespace

            extern "C" {{

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

    def _wrapper_utils(self):
        return text.code(
            """
            template<typename T>
            T lua_tointegral(lua_State *L, int arg)
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
            T lua_tofloating(lua_State *L, int arg)
            {
              int valid = 0;
              auto f = lua_tonumberx(L, arg, &valid);

              luaL_argcheck(L, valid, arg,
                            "not convertible to lua_Number");
              luaL_argcheck(L, f >= std::numeric_limits<T>::min(), arg,
                            "conversion would underflow");
              luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                            "conversion would overflow");

              return static_cast<T>(f);
            }
            """)

    def _function_check_num_parameters(self, f):
        num_min = f.num_parameters(required_only=True)
        num_max = f.num_parameters()

        if num_min == num_max:
            num = num_min
            return text.code(
                f"""
                if (lua_gettop(L) != {num})
                  return luaL_error(L, "function expects {num} arguments");
                """)

        return text.code(
            f"""
            if (lua_gettop(L) < {num_min} || lua_gettop(L) > {num_max})
              return luaL_error(L, "function expects between {num_min} and {num_max} arguments");
            """)

    def _function_declare_parameters(self, f):
        if f.num_parameters() == 0:
            return

        declarations = []

        for p in f.parameters():
            p_type = p.type()

            if p_type.is_reference():
                p_type = p_type.referenced()

            if p_type.is_const():
                p_type = p_type.without_const()

            declaration = f"{p_type} {p.name()}"

            if p.has_default_argument():
                declaration = f"{declaration} = {p.default_argument()}"

            declarations.append(f"{declaration};")

        return '\n'.join(declarations)

    def _function_peek_parameters(self, f):
        if f.num_parameters() == 0:
            return

        return '\n'.join(self._function_peek_parameter(p, i)
                         for i, p in enumerate(f.parameters(), start=1))

    def _function_peek_parameter(self, p, i):
        p_type = p.type()

        if p_type.is_reference():
            p_type = p_type.referenced()

        # check
        lua_type_encoding = self._lua_type_encoding(p_type)
        check_type = f"luaL_checktype(L, {i}, {lua_type_encoding})"

        # peek
        if p_type.is_fundamental('bool'):
            to_type = f"lua_toboolean(L, {i})"
        elif p_type.is_integral():
            to_type = f"lua_tointegral<{p_type}>(L, {i})"
        elif p_type.is_floating():
            to_type = f"lua_tofloating<{p_type}>(L, {i})"
        elif p_type.is_pointer():
            to_type = f"lua_touserdata(L, {i})"
        else:
            raise ValueError('TODO')

        # cast
        cast_type = None

        if p_type.is_pointer():
            cast_type = f"static_cast<{p_type}>"

        if cast_type is not None:
            to_type = f"{cast_type}({to_type})"

        if p.has_default_argument():
            # optional
            peek = text.code(
                f"""
                if (lua_gettop(L) >= {i}) {{
                  {check_type};
                  {p.name()} = {to_type};
                }}
                """)
        else:
            peek = text.code(
                f"""
                {check_type};
                {p.name()} = {to_type};
                """)

        return peek

    def _function_forward(self, f):
        forward_parameters = []

        for p in f.parameters():
            p_type = p.type()

            if p_type.is_rvalue_reference():
                p_str = f"std::move({p.name()})"
            else:
                p_str = p.name()

            forward_parameters.append(p_str)

        forward_parameters = ', '.join(p for p in forward_parameters)

        forward = f"{f.name()}({forward_parameters})"

        return_type = f.return_type()

        if not return_type.is_void():
            if return_type.is_lvalue_reference():
                return_type = return_type.referenced()

            lua_return_type = self._lua_type(return_type)

            forward = f"lua_push{lua_return_type}(L, {forward})"

        return f"{forward};"

    def _function_return_references(self, f):
        ref_returns = []

        for p in self._function_non_const_lvalue_reference_parameters(f):
            lua_type = self._lua_type(p.type().referenced())

            ref_returns.append(f"lua_push{lua_type}(L, {p.name()});")

        return '\n'.join(ref_returns)

    def _function_num_returns(self, f):
        num_regular_returns = int(not f.return_type().is_void())
        num_reference_returns = len(self._function_non_const_lvalue_reference_parameters(f))

        return num_regular_returns + num_reference_returns

    def _function_non_const_lvalue_reference_parameters(self, f):
        ref_parameters = []

        for p in f.parameters():
            p_type = p.type()

            if not p_type.is_reference():
                continue

            p_type = p_type.referenced()

            if p_type.is_const():
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
                {{
                  luaL_newlib(L, lib);
                }}
                """,
                module=self._module(),
                module_function_table='\n'.join(module_function_table))

    def _module_properties(self):
        module_properties = []

        for v in self.variables():
            lua_name = self._lua_variable_name(v)
            lua_type = self._lua_type(v.type())

            module_properties.append(text.code(
                f"""
                lua_pushstring(L, "{lua_name}");
                lua_push{lua_type}(L, static_cast<{v.type()}>({v.name()}));
                lua_settable(L, -3);
                """,
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

    @staticmethod
    def _lua_type(t):
        if t.is_fundamental('bool'):
            return 'boolean'
        elif t.is_integral():
            return 'integer'
        elif t.is_floating():
            return 'number'
        elif t.is_pointer():
            return 'lightuserdata'
        else:
            raise ValueError('TODO')

    @staticmethod
    def _lua_type_encoding(t):
        if t.is_fundamental('bool'):
            return 'LUA_TBOOLEAN'
        elif t.is_integral() or t.is_floating():
            return 'LUA_TNUMBER'
        elif t.is_pointer():
            return 'LUA_TLIGHTUSERDATA'
        else:
            raise ValueError('TODO')

    @staticmethod
    def _lua_variable_name(v):
        return v.name(qualifiers='replace', case='snake-cap-all')

    @staticmethod
    def _lua_function_name(f):
        return f.overload_name(qualifiers='replace', case='snake')

    @staticmethod
    def _lua_parameter_name(p):
        return p.name(case='snake')
