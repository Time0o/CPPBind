import type_info
import lua_util

from backend import Backend
from cppbind import Options
from text import code


class LuaBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        self._wrapper_module = self.output_file(
            self.input_file().modified(filename='{filename}_lua', ext='cpp-source'))

    def wrap_before(self):
        self._wrapper_module.append(code(
            """
            #define LUA_LIB

            extern "C"
            {{
              #include "lua.h"
              #include "lauxlib.h"
            }}

            {input_includes}

            {type_info_include}

            {type_info_type_instances}

            {lua_util_include}

            {forward_declarations}
            """,
            input_includes='\n'.join(self.input_includes()),
            type_info_include=type_info.path().include(),
            type_info_type_instances=type_info.type_instances(),
            forward_declarations=self._function_forward_declarations(),
            lua_util_include=lua_util.path().include()))

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(code(
            """
            namespace
            {{

            {register_module}

            {create_metatables}

            }} // namespace

            extern "C"
            {{

            LUALIB_API int luaopen_{module_name}(lua_State *L)
            {{
              __register(L);
              __createmetatables(L);

              return 1;
            }}

            }} // extern "C"
            """,
            register_module=self._register(constants=self.constants(),
                                           functions=self.functions(),
                                           records=self.records()),
            create_metatables=self._create_metatables(self.records()),
            module_name=self._module_name()))

    def wrap_constant(self, c):
        pass

    def wrap_record(self, r):
        if r.is_abstract():
            return

        self._wrapper_module.append(code(
            f"""
            namespace __{r.name_target()}
            {{{{

            {{function_definitions}}

            {{register}}

            }}}} // namespace __{r.name_target()}
            """,
            function_definitions=self._function_definitions(
                [f for f in r.functions() if not f.is_destructor()]),
            register=self._register(
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
                namespace __{r.name_target()}
                {{{{

                {{function_declarations}}

                }}}} // namespace __{r.name_target()}
                """,
                function_declarations=self._function_declarations(r.functions())))

        return '\n\n'.join(forward_declarations)

    @classmethod
    def _function_declaration(cls, f):
        return f"int {f.name_target()}(lua_State *L);"

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
        return f"int {f.name_target()}(lua_State *L)"

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
        num_min = sum(1 for p in f.parameters() if p.default_argument() is None)
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
    def _register(cls, constants=[], functions=[], records=[]):
        return code(
            f"""
            void __register(lua_State *L)
            {{{{
              lua_createtable(L, 0, {len(functions)});

              {{register_constants}}

              {{register_functions}}

              {{register_records}}
            }}}}
            """,
            register_constants=cls._register_constants(constants),
            register_functions=cls._register_functions(functions),
            register_records=cls._register_records(records))

    @classmethod
    def _register_constants(cls, constants):
        if not constants:
            return "// no constants"

        def register_constant(v):
            # XXX generalize
            if v.type().is_enum():
                arg = f"static_cast<{v.type().underlying_integer_type()}>({v.name()})"
                push = f"{lua_util.pushintegral(arg, constexpr=True)}"
            elif v.type().is_integral():
                push = f"{lua_util.pushintegral(v.name(), constexpr=True)}"
            elif v.type().is_floating():
                push = f"{lua_util.pushfloating(v.name(), constexpr=True)}"

            return code(
                f"""
                lua_pushstring(L, "{v.name_target()}");
                {{push}};
                lua_settable(L, -3);
                """,
                push=push)

        return '\n\n'.join(map(register_constant, constants))

    @staticmethod
    def _register_functions(functions):
        if not functions:
            return "// no functions"

        def function_entry(f):
            if f.is_member():
                return f'{{"{f.name_target(qualified=False)}", {f.name_target()}}}'
            else:
                return f'{{"{f.name_target()}", {f.name_target()}}}'

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
                __{r.name_target()}::__register(L);
                lua_setfield(L, -2, "{r.name_target()}");
                """)

        return '\n\n'.join(map(register_record, records))

    @staticmethod
    def _create_metatables(records):
        create = []

        for r in records:
            if r.is_abstract():
                continue

            create.append(lua_util.createmetatable(r))

        return code(
            """
            void __createmetatables(lua_State *L)
            {{
              {create}
            }}
            """,
            create='\n\n'.join(create))
