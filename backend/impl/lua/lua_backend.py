import lua_util
import type_info
from backend import Backend
from cppbind import Identifier as Id, Options
from lua_patcher import LuaPatcher
from lua_type_translator import LuaTypeTranslator
from text import code


class LuaBackend(Backend('lua')):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._patcher = LuaPatcher()
        self._type_translator = LuaTypeTranslator()

    def patcher(self):
        return self._patcher

    def type_translator(self):
        return self._type_translator

    def wrap_before(self):
        self._wrapper_module = self.output_file(
            self.input_file().modified(filename='{filename}_lua', ext='cpp-source'))

        self._wrapper_module.append(code(
            """
            #define LUA_LIB

            {lua_includes}

            {input_includes}

            {type_info_include}

            {type_info_type_instances}

            {lua_util_include}

            namespace
            {{
            """,
            lua_includes=self._lua_includes(),
            input_includes='\n'.join(self.includes()),
            type_info_include=type_info.path().include(),
            type_info_type_instances=type_info.type_instances(),
            lua_util_include=lua_util.path().include()))

    def wrap_after(self):
        ## XXX support different Lua versions
        self._wrapper_module.append(code(
            """
            {register_module}

            {create_metatables}

            extern "C"
            {{

            LUALIB_API int luaopen_{module_name}(lua_State *L)
            {{
              __register(L);
              __createmetatables(L);

              return 1;
            }}

            }} // extern "C"

            }} // namespace
            """,
            register_module=self._register(
                constants=self.constants(include_definitions=True,
                                         include_enums=True),
                functions=self.functions(),
                records=self.records()),
            create_metatables=self._create_metatables(self.records()),
            module_name=self._module_name()))

    def wrap_constant(self, c):
        pass

    def wrap_record(self, r):
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

    def _lua_includes(self):
        lua_includes = ['lua.h', 'lauxlib.h']

        if Options.lua_include_dir:
            lua_includes = [
                os.path.join(Options.lua_include_dir, inc)
                for inc in lua_includes
            ]

        lua_includes = '\n'.join(f'#include "{inc}"' for inc in lua_includes)

        if not Options.lua_include_cpp:
            lua_includes = code(
                """
                extern "C"
                {{
                  {lua_includes}
                }}
                """,
                lua_includes=lua_includes)

        return lua_includes

    def _function_definition(self, f):
        return code(
            """
            {header}
            {{
              {body}
            }}
            """,
            header=self._function_header(f),
            body=f.forward())

    def _function_definitions(self, functions):
        return '\n\n'.join(map(self._function_definition, functions))

    def _function_header(self, f):
        return f"int {f.name_target()}(lua_State *L)"

    def _module_name(self):
        return self.input_file().filename()

    def _register(self, constants=[], functions=[], records=[]):
        table_num_functions = len(functions)

        table_register = []

        if constants:
            table_register += self._register_constants(constants)

        if functions:
            table_register += [self._register_functions(functions)]

        if records:
            table_register += self._register_records(records)

        return code(
            """
            void __register(lua_State *L)
            {{
              lua_createtable(L, 0, {table_num_functions});

              {table_register}
            }}
            """,
            table_num_functions=table_num_functions,
            table_register='\n\n'.join(table_register))

    def _register_constants(self, constants):
        return [c.assign() for c in constants]

    def _register_functions(self, functions):
        if not functions:
            return "// no functions"

        def function_entry(f):
            if f.is_member():
                entry = f.name_target(quals=Id.REMOVE_QUALS)
            else:
                entry = f.name_target()

            return f'{{"{entry}", {f.name_target()}}}'

        return code(
            """
            static luaL_Reg const __functions[] = {{
              {function_entries},
              {{nullptr, nullptr}}
            }};

            luaL_setfuncs(L, __functions, 0);
            """,
            function_entries=',\n'.join(map(function_entry, functions)))

    def _register_records(self, records):
        def register_record(r):
            return code(
                f"""
                __{r.name_target()}::__register(L);
                lua_setfield(L, -2, "{r.name_target()}");
                """)

        return map(register_record, records)

    def _create_metatables(self, records):
        create = []

        for r in records:
            create.append(lua_util.createmetatable(r))

        return code(
            """
            void __createmetatables(lua_State *L)
            {{
              {create}
            }}
            """,
            create='\n\n'.join(create))
