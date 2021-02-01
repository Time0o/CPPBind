from enum import Enum

from backend import *
from cppbind import Identifier as Id, Type, Variable, Record, Function, Parameter

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

        self._header_include_set = set()
        self._source_include_set = set()

    def wrap_before(self):
        pass

    def wrap_after(self):
        self._wrapper_header.prepend(text.code(
            """
            #ifndef {header_guard}
            #define {header_guard}

            #ifdef __cplusplus
            extern "C" {{
            #endif

            {header_includes}
            """,
            header_guard=self._header_guard(),
            header_includes=self._header_includes()
        ))

        self._wrapper_header.append(text.code(
            """
            #ifdef __cplusplus
            }} // extern "C"
            #endif

            #endif // {header_guard}
            """,
            header_guard=self._header_guard()
        ))

        self._wrapper_source.prepend(text.code(
            """
            {source_includes}

            {input_include}

            extern "C" {{

            {wrapper_include}
            """,
            source_includes=self._source_includes(),
            input_include=text.include(self.input_file().basename()),
            wrapper_include=text.include(self._wrapper_header.basename()),
        ))

        self._wrapper_source.append(text.code(
            """
            } // extern "C"
            """
        ))

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
            f.self_type = f.parent.type.pointer_to()
            f.parameters.insert(0, Parameter(Id.SELF, f.self_type))

        self._wrapper_header.append(self._function_declaration(f))
        self._wrapper_source.append(self._function_definition(f))

    def _header_includes(self):
        headers = [
            '<stdbool.h>'
        ]

        return '\n'.join(f"#include {header}" for header in headers)

    def _source_includes(self):
        headers = [
            '<utility>'
        ]

        return '\n'.join(f"#include {header}" for header in headers)

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
            this = f.self_type.cast(Id.SELF)

        parameters = ', '.join(self._function_parameter_forwardings(f))

        if f.is_constructor():
            body = f"new {f.parent.type}({parameters})"
        elif f.is_destructor():
            body = f"delete {this}"
        elif f.is_instance():
            body = f"{this}->{f.name.format(quals=Id.REMOVE_QUALS)}({parameters})"
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
