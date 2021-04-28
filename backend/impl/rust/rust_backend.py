from backend import Backend, backend, switch_backend
from cppbind import Identifier as Id, Type
from rust_patcher import RustPatcher
from rust_type_translator import RustTypeTranslator
from text import code


class RustBackend(Backend('rust')):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._patcher = RustPatcher()
        self._type_translator = RustTypeTranslator()

        self._modules = {}

    def patcher(self):
        return self._patcher

    def type_translator(self):
        return self._type_translator

    def wrap_before(self):
        with switch_backend('c'):
            backend().run()

        self._wrapper_source = self.output_file(
            self.input_file().modified(filename='{filename}_rust', ext='.rs'))

        self._wrapper_source.append("mod internal {")
        self._wrapper_source.append(self._c_functions())

    def wrap_after(self):
        self._wrapper_source.append("} // mod internal")
        self._wrapper_source.append(self._modules_export())

    def wrap_definition(self, d):
        pass # XXX

    def wrap_enum(self, e):
        if e.is_anonymous():
            for c in e.constants():
                name = c.name_target(case=Id.SNAKE_CASE_CAP_ALL)

                self._wrapper_source.append(
                    f"pub const {name}: {c.type().target()} = {c.value()};")

                self._modules_add(self._module(c), name)
        else:
            enum_constants = [f"{c.name_target()} = {c.value()}"
                              for c in e.constants()]

            self._wrapper_source.append(code(
                """
                #[repr(C)]
                pub enum {enum_name} {{
                    {enum_constants}
                }}
                """,
                enum_name=e.name_target(),
                enum_constants=',\n'.join(enum_constants)))

            self._modules_add(self._module(e), e.name_target())

    def wrap_constant(self, c):
        self._wrapper_source.append(c.assign())
        self._modules_add(self._module(c), f"get_{c.name_target(case=Id.SNAKE_CASE)}")

    def wrap_record(self, r):
        pass

    def wrap_function(self, f):
        self._wrapper_source.append(self._function_definition_rust(f))
        self._modules_add(self._module(f), f.name_target())

    def _c_lib_name(self):
        return f"{self.input_file().filename()}_c"

    def _c_types(self):
        type_imports = set()

        for t in self.types():
            if t.is_fundamental():
                type_imports.add(f'std::os::raw::{t.target_c(scoped=False)}')
            elif t.is_c_string():
                type_imports.add(f'std::ffi::CStr')
                type_imports.add(f'std::ffi::CString')

        return '\n'.join(f'pub use {ti};' for ti in sorted(type_imports))

    def _c_functions(self):
        c_constant_declarations = [c.declare() for c in self.constants()]

        c_function_declarations = [self._function_declaration_c(f)
                                   for f in self.functions()]

        return code(
            """
            mod c {{
                {c_types}

                #[link(name="{c_lib_name}")]
                extern "C" {{
                    {c_constant_declarations}

                    {c_function_declarations}
                }}
            }}
            """,
            c_types=self._c_types(),
            c_lib_name=self._c_lib_name(),
            c_constant_declarations='\n'.join(c_constant_declarations),
            c_function_declarations='\n'.join(c_function_declarations))

    def _function_declaration_c(self, f):
        return f"{self._function_header_c(f)};"

    def _function_definition_rust(self, f):
        return code(
            """
            {header} {{
                {body}
            }}
            """,
            header=self._function_header_rust(f),
            body=self._function_body_rust(f))

    def _function_header_c(self, f):
        name = f.name_target(quals=Id.REPLACE_QUALS)

        param_declare = lambda p: f"{p.name_target()}: {p.type().target_c(scoped=False)}"
        params = ', '.join(param_declare(p) for p in f.parameters())

        header = f"pub fn {name}({params})"

        if not f.return_type().is_void():
            header += f" -> {f.return_type().target_c(scoped=False)}"

        return header

    def _function_header_rust(self, f):
        name = f.name_target()

        has_lifetime = lambda t: t.is_reference() or t.is_c_string()

        if any(has_lifetime(p.type()) for p in f.parameters()) or has_lifetime(f.return_type()):
            name = f"{name}<'a>"

        param_declare = lambda p: f"{p.name_target()}: {p.type().target()}"
        params = ', '.join(param_declare(p) for p in f.parameters())

        header = f"pub unsafe fn {name}({params})"

        if not f.return_type().is_void():
            header += f" -> {f.return_type().target()}"

        return header

    def _function_body_rust(self, f):
        return f.forward()

    def _module(self, obj):
        return obj.name().qualifiers().format(case=Id.SNAKE_CASE, quals=Id.REPLACE_QUALS)

    def _modules_add(self, mod, symbol):
        self._modules[mod] = self._modules.get(mod, []) + [symbol]

    def _modules_export(self):
        for mod, symbols in self._modules.items():
            use = [f"pub use super::internal::{s};" for s in symbols]

            return code(
                """
                pub mod {mod_name} {{
                    {mod_use}
                }} // mod {mod_name}
                """,
                mod_name=mod,
                mod_use='\n'.join(use))
