from backend import *
from cppbind import Identifier as Id, Type, Variable, Record, Function, Parameter

from text import code


def _name_lua(get=lambda self: self.name, case=Id.SNAKE_CASE, quals=Id.REPLACE_QUALS):
    @property
    def _name_lua_closure(self):
        return get(self).format(case=case, quals=quals)

    return _name_lua_closure

Variable.name_lua = _name_lua(case=Id.SNAKE_CASE_CAP_ALL)
Function.name_lua = _name_lua(get=lambda self: self.name_overloaded)
Function.name_unqualified_lua = _name_lua(get=lambda self: self.name_overloaded,
                                          quals=Id.REMOVE_QUALS)
Parameter.name_lua = _name_lua()
Record.name_lua = _name_lua(case=Id.PASCAL_CASE)


@property
def _type_lua(self):
    if self.is_integral() or self.is_enum():
        return 'integer'
    elif self.is_floating():
        return 'number'
    elif self.is_boolean():
        return 'boolean'
    elif self.is_pointer():
        return 'userdata'
    else:
        raise ValueError('TODO')

@property
def _type_intermediate(self):
    return self.referenced().without_const()

Type.lua = _type_lua
Type.intermediate = _type_intermediate


@property
def _type_lua_encoding(self):
    if self.is_integral() or self.is_floating() or self.is_enum():
        return 'LUA_TNUMBER'
    elif self.is_boolean():
        return 'LUA_TBOOLEAN'
    elif self.is_pointer():
        return 'LUA_TLIGHTUSERDATA'
    else:
        raise ValueError('TODO')

Type.lua_encoding = _type_lua_encoding


class LuaBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        self._wrapper_module = self.output_file(
            self.input_file().modified(filename='{filename}_lua', ext='.cpp'))

    def wrap_before(self):
        self._wrapper_module.append(code(
            f"""
            #define LUA_LIB

            #include <limits>
            #include <utility>

            extern "C"
            {{
              #include "lua.h"
              #include "lauxlib.h"
            }}

            {self.input_file().include()}
            """))

        self._wrapper_module.append(self._util())

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(code(
            """
            {register_module}

            extern "C"
            {{

            LUALIB_API int luaopen_{module_name}(lua_State *L)
            {{
              __register(L);

              return 1;
            }}

            }} // extern "C"
            """,
            register_module=self._register(variables=self.variables(),
                                           functions=self.functions(),
                                           records=self.records()),
            module_name=self._module_name()))

    def wrap_variable(self, c):
        pass

    def wrap_record(self, r):
        for f in r.functions:
            if f.is_constructor() or f.is_instance():
                f.parameters.insert(0, Parameter.self(f.parent.type.pointer_to(1)))

        self._wrapper_module.append(code(
            f"""
            namespace __{r.name_lua}
            {{{{

            {{declare_destructor}}

            {{define_functions}}

            {{register_record}}

            }}}} // namespace __{r.name_lua}
            """,
            declare_destructor=self._function_declare(r.destructor),
            define_functions=self._functions_define(r.functions),
            register_record=self._register(
                variables=r.variables,
                functions=[f for f in r.functions if not f.is_destructor()])))

    def wrap_function(self, f):
        self._wrapper_module.append(self._function_define(f))

    @staticmethod
    def _util():
        return code(
            """
            namespace __util
            {

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
              luaL_argcheck(L, f >= std::numeric_limits<T>::lowest(), arg,
                            "conversion would underflow");
              luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                            "conversion would overflow");

              return static_cast<T>(f);
            }

            template<typename T>
            void lua_pushintegral(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Integer>::min() ||
                  val > std::numeric_limits<lua_Integer>::max())
                luaL_error(L, "result not representable by lua_Integer");

              lua_pushinteger(L, val);
            }

            template<typename T>
            void lua_pushfloating(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Number>::lowest() ||
                  val > std::numeric_limits<lua_Number>::max())
                luaL_error(L, "result not representable by lua_Number");

              lua_pushnumber(L, val);
            }

            } // namespace __util

            #define __util_lua_pushintegral_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Integer>::min() && \\
                            VAL <= std::numeric_limits<lua_Integer>::max(), \\
                            "parameter not representable by lua_Integer"); \\
              lua_pushinteger(L, VAL);


            #define __util_lua_pushfloating_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \\
                            VAL <= std::numeric_limits<lua_Number>::max(), \\
                            "parameter not representable by lua_Number"); \\
              lua_pushnumber(L, VAL);
            """)

    @classmethod
    def _function_declare(cls, f):
        return f"int {f.name_lua}(lua_State *L);"

    @classmethod
    def _functions_define(cls, fs):
        return '\n\n'.join(map(cls._function_define, fs))

    @classmethod
    def _function_define(cls, f):
        return code(
            f"""
            int {f.name_lua}(lua_State *L)
            {{{{
              {{check_num_parameters}}

              {{declare_parameters}}

              {{peek_parameters}}

              {{execute_function}}

              {{return_references}}

              return {{num_returns}};
            }}}}
            """,
            check_num_parameters=cls._function_check_num_parameters(f),
            declare_parameters=cls._function_declare_parameters(f),
            peek_parameters=cls._function_peek_parameters(f),
            execute_function=cls._function_execute(f),
            return_references=cls._function_return_references(f),
            num_returns=cls._function_num_returns(f))

    @staticmethod
    def _function_check_num_parameters(f):
        num_min = sum(1 for p in f.parameters if not p.default_argument)
        num_max = len(f.parameters)

        if num_min == num_max:
            return code(
                f"""
                if (lua_gettop(L) != {num_min})
                  return luaL_error(L, "function expects {num_min} arguments");
                """)

        return code(
            f"""
            if (lua_gettop(L) < {num_min} || lua_gettop(L) > {num_max})
              return luaL_error(L, "function expects between {num_min} and {num_max} arguments");
            """)

    @staticmethod
    def _function_declare_parameters(f):
        if not f.parameters:
            return

        def declare(p):
            if p.is_self():
                return f"{p.type} {p.name};"

            declaration = f"{p.type.intermediate} {p.name}"

            if p.default_argument is not None:
                default = f"{p.default_argument}"

                if p.type.intermediate.is_scoped_enum():
                    default = f"static_cast<{p.type.intermediate}>({default})"

                declaration = f"{declaration} = {default}"

            return f"{declaration};"

        return '\n'.join(map(declare, f.parameters))

    @classmethod
    def _function_peek_parameters(cls, f):
        peek_parameters = '\n\n'.join(
            cls._function_peek_parameter(f, p, i)
            for i, p in enumerate(f.parameters, start=1))

        if any(p.default_argument for p in f.parameters):
            peek_parameters = code(
              """
              do {{
                {peek_parameters}
              }} while(false);
              """,
              peek_parameters=peek_parameters)

        return peek_parameters

    @classmethod
    def _function_peek_parameter(cls, f, p, i):
        break_default = None
        check_type = None
        peek = None

        if p.is_self():
            check_type = f"luaL_checktype(L, {i}, LUA_TTABLE);"

            if not f.is_constructor():
                peek = code(
                    f"""
                    lua_getfield(L, {i}, "{f.self_parameter.name}");
                    {p.name} = static_cast<{p.type}>(lua_touserdata(L, -1));
                    lua_pop(L, 1);
                    """)
        else:
            if p.default_argument:
                break_default = f"if (lua_gettop(L) < {i}) break;"

            check_type = f"luaL_checktype(L, {i}, {p.type.intermediate.lua_encoding});"

            peek = f"{p.name} = {cls._lua_peek(p.type.intermediate, i)};"

        return code(
            """
            {break_default}
            {check_type}
            {peek}
            """,
            break_default=break_default,
            check_type=check_type,
            peek=peek)

    @classmethod
    def _function_execute(cls, f):
        parameters = cls._function_forward_parameters(f)

        if f.is_constructor():
            return code(
                f"""
                lua_newtable(L);
                lua_pushvalue(L, 1);
                lua_setfield(L, 1, "__index");
                lua_pushcfunction(L, {f.parent.destructor.name_lua});
                lua_setfield(L, 1, "__gc");
                lua_pushvalue(L, 1);
                lua_setmetatable(L, -2);

                {f.self_parameter.name} = static_cast<{f.parent.type} **>(
                  lua_newuserdata(L, sizeof({f.parent.type} *)));

                *{f.self_parameter.name} = new {f.parent.type}({parameters});

                lua_setfield(L, -2, "{f.self_parameter.name}");
                """)
        elif f.is_destructor():
            return code(f"delete *{f.self_parameter.name};")
        else:
            if f.is_instance():
                forward = f"(*{f.self_parameter.name})->{f.name.unqualified()}({parameters})"
            else:
                forward = f"{f.name}({parameters})"

            if not f.return_type.is_void():
                forward = cls._lua_push(f.return_type.referenced(), forward)

            return f"{forward};"

    @staticmethod
    def _function_forward_parameters(f):
        def forward(p):
            if p.type.is_rvalue_reference():
                return f"std::move({p.name})"
            else:
                return f"{p.name}"

        return ', '.join(map(forward, f.non_self_parameters))

    @classmethod
    def _function_return_references(cls, f):
        def return_reference(p):
            push = cls._lua_push(p.type.referenced(), p.name)
            return f"{push};"

        return '\n'.join(map(return_reference, cls._function_inout_parameters(f)))

    @classmethod
    def _function_num_returns(cls, f):
        if f.is_constructor():
            return 1

        return int(not f.return_type.is_void()) + \
               len(cls._function_inout_parameters(f))

    @staticmethod
    def _function_inout_parameters(f):
        def is_inout(p):
            return p.type.is_lvalue_reference() and \
                   not p.type.referenced().is_const()

        return list(filter(is_inout, f.non_self_parameters))

    def _module_name(self):
        return self.input_file().filename()

    @classmethod
    def _register(cls, variables=[], functions=[], records=[]):
        return code(
            f"""
            void __register(lua_State *L)
            {{{{
              lua_createtable(L, 0, {len(functions)});

              {{register_variables}}

              {{register_functions}}

              {{register_records}}
            }}}}
            """,
            register_variables=cls._register_variables(variables),
            register_functions=cls._register_functions(functions),
            register_records=cls._register_records(records))

    @classmethod
    def _register_variables(cls, variables):
        if not variables:
            return "// no variables"

        def register_variable(v):
            return code(
                f"""
                lua_pushstring(L, "{v.name_lua}");
                {{push}};
                lua_settable(L, -3);
                """,
                push=cls._lua_push(v.type, v.name, constexpr=True))

        return '\n\n'.join(map(register_variable, variables))

    @staticmethod
    def _register_functions(functions):
        if not functions:
            return "// no functions"

        def function_entry(f):
            if f.is_member():
                return f'{{"{f.name_unqualified_lua}", {f.name_lua}}}'
            else:
                return f'{{"{f.name_lua}", {f.name_lua}}}'

        return code(
            """
            static luaL_Reg const __functions[] = {{
              {function_entries},
              {{nullptr, nullptr}}
            }};

            luaL_setfuncs(L, __functions, 0);
            """,
            function_entries=',\n'.join(map(function_entry, functions)))

    @staticmethod
    def _register_records(records):
        if not records:
            return "// no records"

        def register_record(r):
            return code(
                f"""
                __{r.name_lua}::__register(L);
                lua_setfield(L, -2, "{r.name_lua}");
                """)

        return '\n\n'.join(map(register_record, records))

    @classmethod
    def _lua_peek(cls, t, i):
        if t.is_scoped_enum():
            it = t.without_enum()
            return f"static_cast<{t}>(__util::lua_tointegral<{it}>(L, {i}))"
        elif t.is_integral():
            return f"__util::lua_tointegral<{t}>(L, {i})"
        elif t.is_floating():
            return f"__util::lua_tofloating<{t}>(L, {i})"
        elif t.is_pointer():
            return f"static_cast<{t}>(lua_to{t.lua}(L, {i}))"
        else:
            return f"lua_to{t.lua}(L, {i})"

    @classmethod
    def _lua_push(cls, t, v, constexpr=False):
        if t.is_scoped_enum():
            it = t.without_enum()
            return f"__util::lua_pushintegral<{it}>(L, static_cast<{it}>({v}))"
        elif t.is_integral():
            if constexpr:
                return f"__util_lua_pushintegral_constexpr(L, {v})"
            else:
                return f"__util::lua_pushintegral<{t}>(L, {v})"
        elif t.is_floating():
            if constexpr:
                return f"__util_lua_pushfloating_constexpr(L, {v})"
            else:
                return f"__util::lua_pushfloating<{t}>(L, {v})"
        elif t.is_pointer():
            return f"lua_pushlightuserdata(L, {v})"
        else:
            return f"lua_push{t.lua}(L, {v})"
