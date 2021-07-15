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

        self._wrapper_source.append(self._c_declarations())

    def wrap_definition(self, d):
        self.wrap_variable(d.as_variable())

    def wrap_enum(self, e):
        if e.is_anonymous():
            for c in e.constants():
                self._wrapper_source.append(
                    f"pub const {c.name_target()}: {c.type().target()} = {c.value()};")
        else:
            enum_constants = []
            for c in e.constants():
                enum_constants.append(f"{c.name_target()} = {c.value()}")

            self._wrapper_source.append(code(
                """
                #[repr(C)]
                pub enum {enum_name} {{
                    {enum_constants}
                }}
                """,
                enum_name=e.name_target(),
                enum_constants=',\n'.join(enum_constants)))

    def wrap_variable(self, v):
        self.wrap_function(v.getter())

        if v.is_assignable():
            self.wrap_function(v.setter())

    def wrap_function(self, f):
        self._wrapper_source.append(self._function_definition_rust(f))

    def wrap_record(self, r):
        if not r.is_redeclaration():
            self._wrapper_source.append(self._record_definition_rust(r))

        if r.is_definition():
            if r.is_polymorphic():
                self._wrapper_source.append(self._record_trait_rust(r))

            if not r.is_abstract():
                self._wrapper_source.append(self._record_impl_rust(r))
                self._wrapper_source.append(self._record_trait_impls_rust(r))

    def _c_lib(self):
        return f"{self.input_file().filename()}_c"

    def _c_types(self):
        type_set = self.types(as_set=True)

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

        for v in self.variables(include_macros=True):
            c_declarations.append(self._function_declaration_c(v.getter()))

            if v.is_assignable():
                c_declarations.append(self._function_declaration_c(v.setter()))

        for f in self.functions(include_members=True):
            c_declarations.append(self._function_declaration_c(f))

        return code(
            """
            mod {c_mod} {{
              #![allow(unused_imports)]
              use super::*;

              #[link(name="{c_lib}")]
              extern "C" {{
                  {c_declarations}
              }}
            }}
            """,
            c_mod=self._c_lib(),
            c_lib=self._c_lib(),
            c_declarations='\n'.join(c_declarations))

    def _rust_module(self):
        return f"{self.input_file().filename()}_rust_internal"

    def _rust_typedefs(self):
        typedefs = []
        for a, t in self.type_aliases():
            typedefs.append(f"type {a.target()} = {t.target()}")

        return typedefs

    def _rust_typedef_types(self):
        return [a.target() for a, _ in self.type_aliases()]

    def _rust_record_types(self):
        record_types = []

        for r in self.records(include_declarations=True):
            if r.is_polymorphic():
                record_types.append(r.trait().name_target())

            record_types.append(r.type().target())

        return record_types

    def _record_definition_rust(self, r):
        record_name = r.name_target()
        record_union = r.union().name_target()
        record_size = r.type().size()

        c_void_ptr = Type('void').pointer_to().target_c()
        c_char = Type('char').target_c()

        return code(
            f"""
            #[repr(C)]
            union {record_union} {{
                mem: [{c_char}; {record_size}],
                ptr: {c_void_ptr},
            }}

            #[repr(C)]
            pub struct {record_name} {{
                obj : {record_union},

                is_const: {c_char},
                is_owning: {c_char},
            }}
            """,
            )

    def _record_impl_rust(self, r):
        record_impl = []

        record_name = r.name_target()
        record_type = r.type().target()
        record_size = r.type().size()

        record_members = []
        for f in r.functions():
            if not (f.is_virtual() or \
                    f.is_copy_constructor() or \
                    f.is_copy_assignment_operator() or \
                    f.is_move_constructor() or \
                    f.is_move_assignment_operator() or \
                    f.is_destructor()):
                record_members.append(self._function_definition_rust(f))

        if record_members:
            record_impl.append(code(
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
                    fn clone_from(&mut self, other: &Self) {{
                        unsafe {{
                            {record_clone_from}
                        }}
                    }}
                    """,
                    record_other=f"{other.name_target()}: {other.type().target()}",
                    record_clone_from=copy_assign.forward()))

            record_impl.append(code(
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
            record_impl.append(code(
                """
                impl Drop for {record_name} {{
                    fn drop(&mut self) {{
                        unsafe {{
                            if self.is_owning == 0{{
                                return;
                            }}

                            {record_destruct}
                        }}
                    }}
                }}
                """,
                record_name=record_name,
                record_destruct=r.destructor().forward()))

        return '\n\n'.join(record_impl)

    def _record_trait_rust(self, r):
        trait_functions = []
        for f in r.functions():
            if not f.is_virtual() or f.is_destructor():
                continue

            if not f.is_overriding():
                trait_functions.append(self._function_declaration_rust(f, is_pub=False))

        trait_bases = [b.trait().name_target()
                       for b in r.bases(recursive=True) if b.is_polymorphic()]

        trait_bases = f": {' + '.join(trait_bases)}" if trait_bases else ""

        return code(
            """
            pub trait {trait_name}{trait_bases} {{
                {trait_functions}
            }}
            """,
            trait_name=r.trait().name_target(),
            trait_bases=trait_bases,
            trait_functions='\n\n'.join(trait_functions))

    def _record_trait_impls_rust(self, r):
        record_name = r.name_target()

        record_trait_impls = []

        for b in [r] + r.bases(recursive=True):
            if not b.is_polymorphic():
                continue

            trait_name = b.trait().name_target()

            trait_functions = []
            for f in r.functions():
                if not f.is_virtual() or f.is_destructor():
                    continue

                if f.origin() == b:
                    trait_functions.append(self._function_definition_rust(f, is_pub=False))

            record_trait_impls.append(code(
                """
                impl {trait_name} for {record_name} {{
                    {trait_functions}
                }}
                """,
                record_name=record_name,
                trait_name=trait_name,
                trait_functions='\n\n'.join(trait_functions)))

        return '\n\n'.join(record_trait_impls)

    def _function_declaration_c(self, f):
        return f"{self._function_header_c(f)};"

    def _function_header_c(self, f):
        name = f.name_target(namespace='keep')

        def param_declare(p):
            t = p.type()
            if t.is_record():
                t = t.with_const().pointer_to()

            return f"{p.name_target()}: {t.target_c()}"

        params = ', '.join(param_declare(p) for p in f.parameters())

        return_annotation = self._function_return_c(f)

        return f"pub fn {name}({params}){return_annotation}"

    def _function_return_c(self, f):
        return_type = f.return_type()

        if return_type.is_void():
            return ""

        if return_type.is_record_indirection():
            return_type = return_type.pointee().unqualified()

        return f" -> {return_type.target_c()}"

    def _function_declaration_rust(self, f, is_pub=True, is_unsafe=True):
        return f"{self._function_header_rust(f, is_pub=is_pub, is_unsafe=is_unsafe)};"

    def _function_definition_rust(self, f, is_pub=True, is_unsafe=True):
        return code(
            """
            {header} {{
                {body}
            }}
            """,
            header=self._function_header_rust(f, is_pub=is_pub, is_unsafe=is_unsafe),
            body=self._function_body_rust(f))

    def _function_header_rust(self, f, is_pub=True, is_unsafe=True):
        if f.is_member():
            name = f.name_target(quals=Id.REMOVE_QUALS)
        else:
            name = f.name_target()

        def param_declare(p):
            t = p.type()

            if p.is_self():
                if t.pointee().is_const():
                    return "&self"

                return "&mut self"

            if t.is_record():
                t = t.with_const().lvalue_reference_to().non_polymorphic()

            return f"{p.name_target()}: {t.target()}"

        class LifetimeCounter():
            def __enter__(self):
                RustTypeTranslator._lifetime_counter = 0
                return self

            def __exit__(self, type, value, traceback):
                RustTypeTranslator._lifetime_counter = None

            def argument_list(self):
                if RustTypeTranslator._lifetime_counter == 0:
                    return None

                r = range(RustTypeTranslator._lifetime_counter)

                return '<' + ', '.join(f"'_{i}" for i in r) + '>'

        with LifetimeCounter() as lifetime_counter:
            params = ', '.join(param_declare(p) for p in f.parameters())

            return_annotation = self._function_return_rust(f)

            lifetime_argument_list = lifetime_counter.argument_list()

        if lifetime_argument_list is not None:
            name = f"{name}{lifetime_argument_list}"

        header = f"fn {name}({params}){return_annotation}"

        if is_unsafe:
            header = f"unsafe {header}"

        if is_pub:
            header = f"pub {header}"

        return header

    def _function_body_rust(self, f):
        return f.forward()

    def _function_return_rust(self, f):
        return_type = f.return_type()

        if return_type.is_void() and f.is_noexcept():
            return ""

        if return_type.is_record_indirection():
            return_type = return_type.pointee().unqualified()

        return_type = return_type.target()

        if f.is_noexcept():
            return f" -> {return_type}"

        return f" -> Result<{return_type}, BindError>"
