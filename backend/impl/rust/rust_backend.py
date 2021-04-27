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

    def patcher(self):
        return self._patcher

    def type_translator(self):
        return self._type_translator

    def wrap_before(self):
        with switch_backend('c'):
            backend().run()

        self._wrapper_source = self.output_file(
            self.input_file().modified(filename='{filename}_rust', ext='.rs'))

        self._wrapper_source.append(self._declarations_c())
        self._wrapper_source.append(self._definitions_rust())

    def wrap_after(self):
        pass

    def wrap_definition(self, d):
        pass # XXX

    def wrap_enum(self, e):
        if e.is_anonymous():
            for c in e.constants():
                self._wrapper_source.append(
                    f"pub const {c.name_target(case=Id.SNAKE_CASE_CAP_ALL)}: {c.type().target()} = {c.value()};")
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

    def wrap_constant(self, c):
        self._wrapper_source.append(c.assign())

    def wrap_record(self, r):
        pass

    def wrap_function(self, f):
        pass

    def _c_lib_name(self):
        return f"{self.input_file().filename()}_c"

    def _c_types(self):
        type_imports = set()

        for t in self.types():
            if t.is_fundamental():
                type_imports.add(f'std::os::raw::{t.target_c(scoped=False)}')

        return '\n'.join(f'pub use {ti};' for ti in sorted(type_imports))

    def _declarations_c(self):
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

    def _definitions_rust(self):
        function_definitions = [self._function_definition_rust(f)
                                for f in self.functions()]

        return '\n\n'.join(function_definitions)

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
        parameters = ', '.join(f"{p.name_target()}: {p.type().target_c(scoped=False)}"
                               for p in f.parameters())

        header = f"pub fn {f.name_target()}({parameters})"

        if not f.return_type().is_void():
            header += f" -> {f.return_type().target_c(scoped=False)}"

        return header

    def _function_header_rust(self, f):
        parameters = ', '.join(f"{p.name_target()}: {p.type().target()}"
                               for p in f.parameters())

        header = f"pub unsafe fn {f.name_target()}({parameters})"

        if not f.return_type().is_void():
            header += f" -> {f.return_type().target()}"

        return header

    def _function_body_rust(self, f):
        return f.forward()
