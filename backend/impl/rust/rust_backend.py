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
        self._wrapper_source.append(self._c_declarations())

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
        self._wrapper_source.append(self._record_definition_rust(r))
        self._modules_add(self._module(r), r.name_target())

    def _record_definition_rust(self, r):
        record_name = r.name_target()
        record_union = f"{record_name}Union"
        record_type = r.type().target()
        record_size = r.type().size()

        record_members = []
        for f in r.functions():
            if not (f.is_copy_constructor() or \
                    f.is_copy_assignment_operator() or \
                    f.is_move_constructor() or \
                    f.is_move_assignment_operator() or \
                    f.is_destructor()):
                record_members.append(self._function_definition_rust(f))

        c_void_ptr = Type('void').pointer_to().target_c()
        c_char = Type('char').target_c()

        record_definition = []

        record_definition.append(code(
            f"""
            #[repr(C)]
            union {record_union} {{
                mem: [{c_char}; {record_size}],
                ptr: {c_void_ptr},
            }}

            #[repr(C)]
            pub struct {record_name} {{
                obj : {record_union},
                is_initialized: {c_char},
                is_const: {c_char},
                is_owning: {c_char},
            }}
            """,
            ))

        record_definition.append(code(
            """
            impl {record_name} {{
                {record_members}
            }}
            """,
            record_name=record_name,
            record_members='\n\n'.join(record_members)))

        if r.is_copyable():
            record_clone = []

            record_clone.append(code(
                """
                fn clone(&self) -> {record_type} {{
                    unsafe {{
                        {record_clone}
                    }}
                }}
                """,
                record_type=record_type,
                record_clone=r.copy_constructor().forward()))

            copy_assign = r.copy_assignment_operator()

            if copy_assign is not None:
                other = copy_assign.parameters()[1]

                record_clone.append(code(
                    """
                    fn clone_from<'a>(&mut self, {record_other}) {{
                        unsafe {{
                            {record_clone_from};
                        }}
                    }}
                    """,
                    record_other=f"{other.name_target()}: {other.type().target()}",
                    record_clone_from=copy_assign.forward()))

            record_definition.append(code(
                """
                impl Clone for {record_name} {{
                    {record_clone}
                }}
                """,
                record_name=record_name,
                record_type=record_type,
                record_clone='\n\n'.join(record_clone)))

        if r.is_moveable():
            pass # XXX

        if r.destructor():
            record_definition.append(code(
                """
                impl Drop for {record_name} {{
                    fn drop(&mut self) {{
                        unsafe {{
                            {record_destruct};
                        }}
                    }}
                }}
                """,
                record_name=record_name,
                record_destruct=r.destructor().forward()))

        return '\n\n'.join(record_definition)

    def wrap_function(self, f):
        self._wrapper_source.append(self._function_definition_rust(f))
        self._modules_add(self._module(f), f.name_target())

    def _c_lib_name(self):
        return f"{self.input_file().filename()}_c"

    def _c_types(self):
        type_set = self.type_set()

        if self.records():
            type_set.add(Type('void'))
            type_set.add(Type('char'))

        type_imports = set()

        for t in sorted(type_set):
            if t.is_fundamental():
                type_imports.add(f'std::os::raw::{t.target_c(scoped=False)}')
            elif t.is_c_string():
                type_imports.add(f'std::ffi::CStr')
                type_imports.add(f'std::ffi::CString')

        return '\n'.join(f'pub use {ti};' for ti in sorted(type_imports))

    def _c_declarations(self):
        c_declarations = []
        c_declarations += [c.declare() for c in self.constants()]
        c_declarations += [self._function_declaration_c(f) for f in self.functions()]
        c_declarations += [self._function_declaration_c(f) for r in self.records() for f in r.functions()]

        return code(
            """
            mod c {{
                #[allow(unused_imports)]
                use super::*;

                {c_types}

                #[link(name="{c_lib_name}")]
                extern "C" {{
                    {c_declarations}
                }}
            }}
            """,
            c_types=self._c_types(),
            c_lib_name=self._c_lib_name(),
            c_declarations='\n'.join(c_declarations))

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

        def param_declare(p):
            t = p.type()
            if t.is_record():
                t = t.pointer_to()

            return f"{p.name_target()}: {t.target_c(scoped=False)}"

        params = ', '.join(param_declare(p) for p in f.parameters())

        header = f"pub fn {name}({params})"

        return_type = f.return_type()

        if not return_type.is_void():
            if return_type.is_record_indirection():
                return_type = return_type.pointee().unqualified()

            header += f" -> {return_type.target_c(scoped=False)}"

        return header

    def _function_header_rust(self, f):
        name = f.name_target()

        has_lifetime = lambda t: t.is_reference() or t.is_record or t.is_c_string()

        if any(has_lifetime(p.type()) for p in f.parameters()) or has_lifetime(f.return_type()):
            name = f"{name}<'a>"

        def param_declare(p):
            t = p.type()

            if p.is_self():
                if t.pointee().is_const():
                    return "&self"

                return "&mut self"

            elif t.is_record():
                t = t.lvalue_reference_to()

            return f"{p.name_target()}: {t.target()}"

        params = ', '.join(param_declare(p) for p in f.parameters())

        header = f"pub unsafe fn {name}({params})"

        return_type = f.return_type()

        if not return_type.is_void():
            if return_type.is_record_indirection():
                return_type = return_type.pointee().unqualified()

            header += f" -> {return_type.target()}"

        return header

    def _function_body_rust(self, f):
        return f.forward()

    def _module(self, obj):
        return obj.name().qualifiers().format(case=Id.SNAKE_CASE, quals=Id.REPLACE_QUALS)

    def _modules_add(self, mod, symbol):
        self._modules[mod] = self._modules.get(mod, []) + [symbol]

    def _modules_export(self):
        mods = []

        for mod_name, mod_symbols in self._modules.items():
            mod_use = [f"pub use super::internal::{s};" for s in mod_symbols]

            mods.append(code(
                """
                pub mod {mod_name} {{
                    {mod_use}
                }} // mod {mod_name}
                """,
                mod_name=mod_name,
                mod_use='\n'.join(mod_use)))

        return '\n\n'.join(mods)
