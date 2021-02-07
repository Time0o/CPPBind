from enum import Enum

from backend import *
from cppbind import Identifier as Id, Type, Variable, Record, Function, Parameter, Typeinfo

Type.__str__ = lambda self: self.str()

import text


def _name_c(get=lambda self: self.name, case=Id.SNAKE_CASE):
    @property
    def _name_c_closure(self):
        return get(self).format(case=case, quals=Id.REPLACE_QUALS)

    return _name_c_closure

Variable.name_c = _name_c(case=Id.SNAKE_CASE_CAP_ALL)
Function.name_c = _name_c(get=lambda self: self.name_overloaded)
Parameter.name_c = _name_c()


def _type_c(get=lambda self: self.type):
    @property
    def _type_c_closure(self):
        t = get(self)

        if t.is_lvalue_reference():
            t = t.referenced().pointer_to()
        elif t.is_rvalue_reference():
            t = t.referenced()

        if t.base.is_record():
            t.str = lambda: (t.format(case=Id.SNAKE_CASE, quals=Id.REPLACE_QUALS)
                              .replace('class', 'struct', 1))

        elif t.base.is_enum():
            t = t.without_enum()

        return t

    return _type_c_closure

Variable.type_c = _type_c()
Record.type_c = _type_c()
Function.return_type_c = _type_c(get=lambda self: self.return_type)
Parameter.type_c = _type_c()


def _type_cast(self, what):
    if self.base.is_record():
        cast = 'reinterpret'
    elif self.base.is_scoped_enum() or self.base.is_integral_underlying_scoped_enum():
        if self.is_lvalue_reference() or self.is_pointer():
            cast = 'reinterpret'
        else:
            cast = 'static'
    else:
        return what

    return f"{cast}_cast<{self}>({what})"

Type.cast = _type_cast


class CBackend(Backend):
    def __init__(self, *args):
        super().__init__(*args)

        input_file = self.input_file()

        self._wrapper_header = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.h'))

        self._wrapper_source = self.output_file(
            input_file.modified(filename='{filename}_c', ext='.cpp'))

    def wrap_before(self):
        self._wrapper_header.append(text.code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            #include <stdbool.h>
            """,
            header_guard=self._header_guard()))

        self._wrapper_source.append(text.code(
            """
            #include <utility>

            {include_input_header}

            {typeinfo_snippet}

            extern "C" {{

            {include_wrapper_header}
            """,
            include_input_header=self.input_file().include(),
            include_wrapper_header=self._wrapper_header.include(),
            typeinfo_snippet=Typeinfo.snippet))

    def wrap_after(self):
        self._wrapper_header.append(text.code(
            """
            #ifdef __cplusplus
            }} // extern "C"
            #endif

            #endif // {header_guard}
            """,
            header_guard=self._header_guard()))

        self._wrapper_source.append(text.code(
            """
            } // extern "C"
            """))

    def wrap_variable(self, v):
        self._wrapper_header.append(text.code(
            f"extern {v.type_c} {v.name_c};"))

        self._wrapper_source.append(text.code(
            f"{v.type_c} {v.name_c} = {v.type_c.cast(v.name)};"))

    def wrap_record(self, r):
        self._wrapper_header.append(text.code(f"{r.type_c};"))

        for f in r.functions:
            self.wrap_function(f)

    def wrap_function(self, f):
        if f.is_constructor():
            f.return_type = f.parent.type.pointer_to()
        elif f.is_instance():
            if f.is_const():
                f.self_type = f.parent.type.with_const().pointer_to()
            else:
                f.self_type = f.parent.type.pointer_to()

            f.parameters.insert(0, Parameter.self(f.self_type))

        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def _header_guard(self):
        guard_id = self.input_file().filename().upper()

        return f"GUARD_{guard_id}_C_H"

    def _function_declaration(self, f):
        header = self._function_header(f)

        return text.code(f"{header};")

    def _function_definition(self, f):
        header = self._function_header(f)
        body = self._function_body(f)

        return text.code(
            f"""
            {header}
            {{ {body} }}
            """)

    def _function_header(self, f):
        parameters = ', '.join(self._function_parameter_declarations(f))

        return f"{f.return_type_c} {f.name_c}({parameters})"

    def _function_body(self, f):
        if f.is_instance():
            this = Id.SELF

        parameters = ', '.join(self._function_parameter_forwardings(f))

        mtp = Typeinfo.make_typed_ptr
        tpc = Typeinfo.typed_ptr_cast

        if f.is_constructor():
            body = mtp(f"new {f.parent.type}({parameters})")
        elif f.is_destructor():
            body = f"delete {tpc(f.parent.type, this)}"
        elif f.is_instance():
            body = f"{tpc(f.parent.type, this)}->{f.name.format(quals=Id.REMOVE_QUALS)}({parameters})"
        else:
            body = f"{f.name}({parameters})"

        if f.return_type.is_void():
            return f"{body};"

        if f.return_type.is_lvalue_reference():
            body = f"&{body}"

        body = f.return_type_c.cast(body)

        return f"return {body};";

    def _function_parameter_declarations(self, f):
        def declaration(p):
            return f"{p.type_c} {p.name_c}"

        return map(declaration, f.parameters)

    def _function_parameter_forwardings(self, f, skip_first=False):
        def forward(p):
            fwd = p.type_c.with_enum().cast(p.name_c)

            if p.type.is_lvalue_reference():
                fwd = f"*{fwd}"
            elif p.type.is_rvalue_reference():
                fwd = f"std::move({fwd})"

            return fwd

        return map(forward, f.parameters[1:] if f.is_instance() else f.parameters)
