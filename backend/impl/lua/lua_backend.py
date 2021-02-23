import type_info
import lua_util as util

from backend import Backend
from cppbind import Identifier as Id, Type, Variable, Record, Function, Parameter
from text import code


def _name_lua(get=lambda self: self.name(), case=Id.SNAKE_CASE, quals=Id.REPLACE_QUALS):
    @property
    def _name_lua_closure(self):
        return get(self).format(case=case, quals=quals)

    return _name_lua_closure

Variable.name_lua = _name_lua(case=Id.SNAKE_CASE_CAP_ALL)
Function.name_lua = _name_lua(get=lambda self: self.name(overloaded=True,
                                                         replace_operator_name=True))
Function.name_unqualified_lua = _name_lua(get=lambda self: self.name(overloaded=True,
                                                                     replace_operator_name=True),
                                          quals=Id.REMOVE_QUALS)
Parameter.name_lua = _name_lua()
Record.name_lua = _name_lua(case=Id.PASCAL_CASE)


class LuaBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        self._wrapper_module = self.output_file(
            self.input_file().modified(filename='{filename}_lua', ext='.cpp'))

    def wrap_before(self):
        self._wrapper_module.append(code(
            """
            #define LUA_LIB

            extern "C"
            {{
              #include "lua.h"
              #include "lauxlib.h"
            }}

            {input_include}

            {type_info_define}

            {forward_declarations}

            {util_define}
            """,
            input_include=self.input_file().include(),
            type_info_define=type_info.define(),
            forward_declarations=self._function_forward_declarations(),
            util_define=util.define()))

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(code(
            """
            namespace
            {{

            {register_module}

            }} // namespace

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
        if r.is_abstract():
            return

        self._wrapper_module.append(code(
            f"""
            namespace __{r.name_lua}
            {{{{

            {{function_definitions}}

            {{register}}

            }}}} // namespace __{r.name_lua}
            """,
            function_definitions=self._function_definitions(
                [f for f in r.functions() if not f.is_destructor()]),
            register=self._register(
                variables=r.variables(),
                functions=[f for f in r.functions() if not f.is_instance()])))

    def wrap_function(self, f):
        self._wrapper_module.append(code(
            """
            namespace
            {{

            {function_definition}

            }} // namespace
            """,
            function_definition=self._function_definition(f)))

    def _function_forward_declarations(self):
        forward_declarations = []

        for r in self.records():
            forward_declarations.append(code(
                f"""
                namespace __{r.name_lua}
                {{{{

                {{function_declarations}}

                }}}} // namespace __{r.name_lua}
                """,
                function_declarations=self._function_declarations(r.functions())))

        return '\n\n'.join(forward_declarations)

    @classmethod
    def _function_declaration(cls, f):
        return f"int {f.name_lua}(lua_State *L);"

    @classmethod
    def _function_declarations(cls, functions):
        return '\n'.join(map(cls._function_declaration, functions))

    @classmethod
    def _function_definition(cls, f):
        return code(
            """
            {header}
            {{
              {body}
            }}
            """,
            header=cls._function_header(f),
            body=cls._function_body(f))

    @classmethod
    def _function_definitions(cls, functions):
        return '\n\n'.join(map(cls._function_definition, functions))

    @classmethod
    def _function_header(cls, f):
        return f"int {f.name_lua}(lua_State *L)"

    @classmethod
    def _function_body(cls, f):
        return code(
            """
            {check_num_parameters}

            {declare_parameters}

            {forward_parameters}

            {forward_call}
            """,
            check_num_parameters=cls._function_check_num_parameters(f),
            declare_parameters=f.declare_parameters(),
            forward_parameters=f.forward_parameters(),
            forward_call=f.forward_call())

    @staticmethod
    def _function_check_num_parameters(f):
        num_min = sum(1 for p in f.parameters() if not p.default_argument)
        num_max = len(f.parameters())

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
            # XXX generalize
            if v.type().is_enum():
                push = f"{util.pushintegral(f'static_cast<{v.type().without_enum()}>({v.name()})', constexpr=True)}"
            elif v.type().is_integral():
                push = f"{util.pushintegral(v.name(), constexpr=True)}"
            elif v.type().is_floating():
                push = f"{util.pushfloating(v.name(), constexpr=True)}"

            return code(
                f"""
                lua_pushstring(L, "{v.name_lua}");
                {{push}};
                lua_settable(L, -3);
                """,
                push=push)

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
        records = [r for r in records if not r.is_abstract()]

        if not records:
            return "// no records"

        def register_record(r):
            return code(
                f"""
                __{r.name_lua}::__register(L);
                lua_setfield(L, -2, "{r.name_lua}");
                """)

        return '\n\n'.join(map(register_record, records))
