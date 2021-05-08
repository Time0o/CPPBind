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

            LUALIB_API int luaopen_{lua_module_name}(lua_State *L)
            {{
              _register(L);
              _createmetatables(L);

              return 1;
            }}

            }} // extern "C"

            }} // namespace
            """,
            lua_module_name=self._lua_module_name(),
            register_module=self._register(
                constants=self.constants(include_definitions=True,
                                         include_enums=True),
                functions=self.functions(),
                records=self.records()),
            create_metatables=self._create_metatables(self.records())))

    def wrap_constant(self, c):
        pass

    def wrap_record(self, r):
        functions = [f for f in r.functions() if not f.is_destructor()]

        self._wrapper_module.append(self._function_definitions(functions))

    def wrap_function(self, f):
        self._wrapper_module.append(self._function_definition(f))

    def _lua_module_name(self):
        return self.input_file().filename()

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

    def _register(self, constants=[], functions=[], records=[]):
        register_objects = []

        if constants:
            register_objects += self._register_constants(constants)

        if functions:
            register_objects += [self._register_functions(functions)]

        if records:
            register_objects += self._register_records(records)

        return code(
            """
            void _register(lua_State *L)
            {{
              lua_createtable(L, 0, {num_functions});

              {register_objects}
            }}
            """,
            num_functions=len(functions),
            register_objects='\n\n'.join(register_objects))

    def _register_constants(self, constants):
        def register_constant(c):
            return code(
                f"""
                lua_pushstring(L, "{c.name_target()}");
                {c.type().output(outp=c.name())}
                lua_settable(L, -3);
                """)

        return [register_constant(c) for c in constants]

    def _register_records(self, records):
        def register_record(r):
            functions = [f for f in r.functions() if not f.is_instance()]

            register = []

            register.append(f"lua_createtable(L, 0, {len(functions)});");

            if functions:
                register.append(self._register_functions(functions))

            register.append(f'lua_setfield(L, -2, "{r.name_target()}");')

            return '\n\n'.join(register)

        return map(register_record, records)

    def _register_functions(self, functions):
        def register_function(f):
            if f.is_member():
                entry_name = f.name_target(quals=Id.REMOVE_QUALS)
            else:
                entry_name = f.name_target()

            entry = f.name_target()

            return f'{{"{entry_name}", {entry}}}'

        return code(
            """
            {{
              static luaL_Reg const _functions[] = {{
                {register},
                {{nullptr, nullptr}}
              }};

              luaL_setfuncs(L, _functions, 0);
            }}
            """,
            register=',\n'.join(map(register_function, functions)))

    def _create_metatables(self, records):
        return code(
            """
            void _createmetatables(lua_State *L)
            {{
              {create_metatables}
            }}
            """,
            create_metatables='\n\n'.join(map(lua_util.createmetatable, records)))
