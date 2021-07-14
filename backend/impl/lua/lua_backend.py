import lua_util
import type_info
from backend import Backend
from cppbind import Enum, Function, Identifier as Id, Options, Record, Type
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

            struct lua_longjmp {{
              struct lua_longjmp *previous;
              int b;
              volatile int status;
            }};

            {lua_util_include}
            {type_info_include}

            {input_includes}

            {type_info_type_instances}

            namespace
            {{
            """,
            lua_includes=self._lua_includes(),
            lua_util_include=lua_util.path().include(),
            input_includes='\n'.join(self.input_includes()),
            type_info_include=type_info.path().include(),
            type_info_type_instances=type_info.type_instances()))

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
            register_module=self._lua_module_register(),
            create_metatables=self._create_metatables(
                r for r in self.records(include_abstract=False))))

    def wrap_definition(self, d):
        self.wrap_variable(d.as_variable())

    def wrap_enum(self, e):
        pass

    def wrap_variable(self, v):
        self.wrap_function(v.getter())

        if v.is_assignable():
            self.wrap_function(v.setter())

    def wrap_function(self, f):
        self._wrapper_module.append(self._function_definition(f))

    def wrap_record(self, r):
        if r.is_abstract():
            return

        functions = [f for f in r.functions() if not f.is_destructor()]

        self._wrapper_module.append(self._function_definitions(functions))

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

    def _lua_module_name(self):
        return self.input_file().filename()

    def _lua_module_register(self):
        return code(
            """
            void _register(lua_State *L)
            {{
              {register}
            }}
            """,
            register=self._register_namespace('_G', self.objects()))

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

    def _register_namespace(self, name, h):
        register = []

        if name == '_G':
            lua_get = 'lua_getglobal(L, "_G");'
        else:
            lua_get = f'lua_getfield(L, -1, "{name}");'

        register.append(code(
            f"""
            {lua_get}
            if (lua_isnil(L, -1)) {{
              lua_pop(L, 1);
              {self._register_createtable(h['__functions'])}
            }}
            """))

        for e in h['__enums']:
            register += self._register_variables(c for c in e.constants())

        if h['__variables'] or h['__macros']:
            variables = h['__variables'] + [m.as_variable() for m in h['__macros']]

            getters = [v.getter() for v in variables]
            register.append(self._register_functions(getters))

            setters = [v.setter() for v in variables if v.is_assignable()]
            if setters:
                register.append(self._register_functions(setters))

        if h['__functions']:
            register.append(self._register_functions(h['__functions']))

        if h['__records']:
            register += self._register_records(h['__records'])

        for name_, h_ in h['__namespaces'].items():
            register.append(self._register_namespace(name_, h_))

        if name == '_G':
            lua_set = 'lua_setglobal(L, "_G");'
        else:
            lua_set = f'lua_setfield(L, -2, "{name}");'

        register.append(lua_set)

        return '\n\n'.join(register)

    def _register_variables(self, variables):
        def register_variable(v):
            variable = v.name()
            if v.type().is_lvalue_reference() and not v.type().is_record_indirection():
                variable = f"&{variable}"

            register = [
                v.type().output(outp=variable),
                self._register_setfield(v.name_target(namespace='remove'))
            ]

            return '\n'.join(register)

        return [register_variable(c) for c in variables]

    def _register_functions(self, functions):
        def register_function(f):
            if f.is_member():
                entry_name = f.name_target(quals=Id.REMOVE_QUALS)
            else:
                entry_name = f.name_target(namespace='remove')

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

    def _register_records(self, records):
        def register_record(r):
            name = r.name_target(namespace='remove')

            static_functions = [f for f in r.functions() if not f.is_instance()]

            register = [self._register_createtable(static_functions)]

            if static_functions:
                register.append(self._register_functions(static_functions))

            register.append(self._register_setfield(name))

            return '\n\n'.join(register)

        return map(register_record, [r for r in records if not r.is_abstract()])

    def _register_createtable(self, functions):
        return f"lua_createtable(L, 0, {len(functions)});"

    def _register_setfield(self, name):
        return f'lua_setfield(L, -2, "{name}");'

    def _create_metatables(self, records):
        return code(
            """
            void _createmetatables(lua_State *L)
            {{
              {create_metatables}
            }}
            """,
            create_metatables='\n\n'.join(map(lua_util.createmetatable, records)))
