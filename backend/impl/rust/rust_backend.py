from backend import Backend, backend, switch_backend
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

        self._wrapper_source.append(self._function_declarations_c(self.functions()))
        self._wrapper_source.append(self._function_definitions_rust(self.functions()))

    def wrap_after(self):
        pass

    def wrap_constant(self, c):
        pass

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
                type_imports.add(f'std::os::raw::{t.target_c()}')

        return '\n'.join(f'use {ti};' for ti in sorted(type_imports))

    def _function_declarations_c(self, fs):
        function_declarations = [self._function_declaration_c(f) for f in fs]

        return code(
            """
            mod c {{
                {c_types}

                #[link(name="{c_lib_name}")]
                extern "C" {{
                    {c_function_declarations}
                }}
            }}
            """,
            c_types=self._c_types(),
            c_lib_name=self._c_lib_name(),
            c_function_declarations='\n'.join(function_declarations))

    def _function_definitions_rust(self, fs):
        function_definitions = [self._function_definition_rust(f) for f in fs]

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
        return self._function_header(f, target='c')

    def _function_header_rust(self, f):
        return self._function_header(f, target='rust')

    def _function_header(self, f, target):
        parameters = ', '.join(f"{p.name_target()}: {p.type().target()}"
                               for p in f.parameters())

        header = f"pub fn {f.name_target()}({parameters})"

        if not f.return_type().is_void():
            header += f" -> {f.return_type().target(target)}"

        return header

    def _function_body_rust(self, f):
        parameters = ', '.join(p.name_target() for p in f.parameters())

        call = f"c::{f.name_target()}({parameters})"

        call_forward = f.return_type().output(outp=call)

        # XXX panic on exception!
        return code(
            """
            unsafe {{
                {call_forward}
            }}
            """,
            call_forward=call_forward)
