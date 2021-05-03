from backend import Backend, backend, switch_backend
from cppbind import Identifier as Id, Type
from itertools import chain
from rust_patcher import RustPatcher
from rust_type_translator import RustTypeTranslator
from text import code


class RustBackend(Backend('rust')):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._patcher = RustPatcher()
        self._type_translator = RustTypeTranslator()

        self._rust_modules = {}

    def patcher(self):
        return self._patcher

    def type_translator(self):
        return self._type_translator

    def wrap_before(self):
        with switch_backend('c'):
            backend().run()

        self._wrapper_source = self.output_file(
            self.input_file().modified(filename='{filename}_rust', ext='.rs'))

        rust_use = [f"use {t};" for t in self._c_types()]

        if not self._rust_is_noexcept():
            rust_use += ["use rust_bind_error::BindError;"]

        rust_typedefs = [f"pub {td};" for td in self._rust_typedefs()]

        self._wrapper_source.append(code(
            """
            #![allow(unused_imports)]

            mod internal {{

            {rust_use}

            {rust_typedefs}
            """,
            rust_use='\n'.join(rust_use),
            rust_typedefs='\n'.join(rust_typedefs)))

    def wrap_after(self):
        c_use = [f"use {t};" for t in self._c_types()]

        c_use += [f"use {self._rust_module()}::internal::{t};"
                  for t in chain(self._rust_record_types(),
                                 self._rust_typedef_types())]

        self._wrapper_source.append(code(
            """
            mod c {{
                {c_use}

                {c_declarations}
            }} // mod c

            }} // mod internal
            """,
            c_use='\n'.join(c_use),
            c_declarations=self._c_declarations()))

        self._wrapper_source.append(self._rust_modules_export())

    def wrap_definition(self, d):
        pass # XXX

    def wrap_enum(self, e):
        if e.is_anonymous():
            for c in e.constants():
                name = c.name_target(case=Id.SNAKE_CASE_CAP_ALL)

                self._wrapper_source.append(
                    f"pub const {name}: {c.type().target()} = {c.value()};")

                self._rust_modules_add(c, name)
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

            self._rust_modules_add(e, e.name_target())

    def wrap_constant(self, c):
        self._wrapper_source.append(c.assign())
        self._rust_modules_add(c, f"get_{c.name_target(case=Id.SNAKE_CASE)}")

    def wrap_record(self, r):
        self._wrapper_source.append(self._record_definition_rust(r))
        self._rust_modules_add(r, r.name_target())

    def wrap_function(self, f):
        self._wrapper_source.append(self._function_definition_rust(f))
        self._rust_modules_add(f, f.name_target())

    def _c_lib(self):
        return f"{self.input_file().filename()}_c"

    def _c_types(self):
        type_set = self.type_set()

        type_set.add(Type('void'))
        type_set.add(Type('char'))

        type_imports = set()

        for t in sorted(type_set):
            if t.is_fundamental():
                type_imports.add(f'std::os::raw::{t.target_c()}')

        type_imports.add(f'std::ffi::CStr')
        type_imports.add(f'std::ffi::CString')

        return list(sorted(type_imports))

    def _c_declarations(self):
        c_declarations = []
        c_declarations += ['pub fn bind_error_what() -> *const c_char;',
                           'pub fn bind_error_reset();']
        c_declarations += [c.declare() for c in self.constants()]
        c_declarations += [self._function_declaration_c(f) for f in self.functions(include_members=True)]

        return code(
            """
            #[link(name="{c_lib}")]
            extern "C" {{
                {c_declarations}
            }}
            """,
            c_lib=self._c_lib(),
            c_declarations='\n'.join(c_declarations))

    def _rust_module(self):
        return f"{self.input_file().filename()}_rust"

    def _rust_modules_add(self, obj, symbol):
        try:
            name = obj.name()
        except:
            name = Id(str(obj))

        mod = name.qualifiers()

        d = self._rust_modules

        if mod:
            for c in mod.components():
                c = c.format(case=Id.SNAKE_CASE)

                if c not in d:
                    d[c] = {}

                d = d[c]

        d['__symbols'] = d.get('__symbols', []) + [symbol]

    def _rust_modules_export(self, modules=None):
        if modules is None:
            modules = self._rust_modules

        export = []

        for mod_name in modules:
            if not mod_name:
                continue

            mod_inner = []

            if mod_name == '__symbols':
                export += [f"pub use {self._rust_module()}::internal::{s};"
                           for s in modules[mod_name]]
            else:
                export.append(code(
                    """
                    pub mod {mod_name} {{
                        {mod_inner}
                    }} // mod {mod_name}
                    """,
                    mod_name=mod_name,
                    mod_inner=self._rust_modules_export(modules[mod_name])))

        return '\n\n'.join(export)

    def _rust_typedefs(self):
        typedefs = []
        for a, t in self.type_aliases():
            self._rust_modules_add(a, a.target())

            typedefs.append(f"type {a.target()} = {t.target()}")

        return typedefs

    def _rust_typedef_types(self):
        return [a.target() for a, _ in self.type_aliases()]

    def _rust_record_types(self):
        return [r.type().target() for r in self.records()]

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
            pass # TODO
            #record_clone = []

            #record_clone.append(code(
            #    """
            #    fn clone(&self) -> {record_type} {{
            #        unsafe {{
            #            {record_clone}
            #        }}
            #    }}
            #    """,
            #    record_type=record_type,
            #    record_clone=r.copy_constructor().forward()))

            #copy_assign = r.copy_assignment_operator()

            #if copy_assign is not None:
            #    other = copy_assign.parameters()[1]

            #    record_clone.append(code(
            #        """
            #        fn clone_from(&mut self, {record_other}) {{
            #            unsafe {{
            #                {record_clone_from};
            #            }}
            #        }}
            #        """,
            #        record_other=f"{other.name_target()}: {other.type().target()}",
            #        record_clone_from=copy_assign.forward()))

            #record_definition.append(code(
            #    """
            #    impl Clone for {record_name} {{
            #        {record_clone}
            #    }}
            #    """,
            #    record_name=record_name,
            #    record_type=record_type,
            #    record_clone='\n\n'.join(record_clone)))

        if r.is_moveable():
            pass # XXX

        if r.destructor():
            record_definition.append(code(
                """
                impl Drop for {record_name} {{
                    fn drop(&mut self) {{
                        unsafe {{
                            {record_destruct}
                        }}
                    }}
                }}
                """,
                record_name=record_name,
                record_destruct=r.destructor().forward()))

        return '\n\n'.join(record_definition)

    def _rust_is_noexcept(self):
        for f in self.functions(include_members=True):
            if (f.is_copy_constructor() or \
                f.is_copy_assignment_operator() or \
                f.is_move_constructor() or \
                f.is_move_assignment_operator()):
                continue

            if not f.is_noexcept():
                return False

        return True

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

            return f"{p.name_target()}: {t.target_c()}"

        params = ', '.join(param_declare(p) for p in f.parameters())

        return_annotation = self._function_return_c(f)

        return f"pub fn {name}({params}){return_annotation}"

    def _function_header_rust(self, f):
        name = f.name_target()

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

        return_annotation = self._function_return_rust(f)

        if '&' in return_annotation:
            name = f"{name}<'a>"

        return f"pub unsafe fn {name}({params}){return_annotation}"

    def _function_return_c(self, f):
        return_type = f.return_type()

        if return_type.is_void():
            return ""

        if return_type.is_record_indirection():
            return_type = return_type.pointee().unqualified()

        return f" -> {return_type.target_c()}"

    def _function_return_rust(self, f):
        return_type = f.return_type()

        if return_type.is_void() and f.is_noexcept():
            return ""

        if return_type.is_record_indirection():
            return_type = return_type.pointee().unqualified()

        return_type = return_type.target()

        return_type = return_type.replace('&', "&'a")

        if f.is_noexcept():
            return f" -> {return_type}"

        return f" -> Result<{return_type}, BindError>"

    def _function_body_rust(self, f):
        return f.forward()
