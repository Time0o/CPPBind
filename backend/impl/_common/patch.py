from cppbind import Function, Parameter, Identifier as Id

from type_translator import TypeTranslator; TT = TypeTranslator()
from util import dotdict
from text import code


def _function_declare_parameters(self):
    if not self.parameters:
        return

    def declare_parameter(p):
        p_type = p.type

        if p_type.is_record():
            p_type = p_type.pointer_to()
        elif p_type.is_reference():
            p_type = p_type.referenced().pointer_to()

        if p_type.is_const():
            p_type = p_type.without_const()

        decl = f"{p_type} {p.name_interm}"

        if p.default_argument is not None:
            default = f"{p.default_argument}"

            if p_type.is_scoped_enum():
                default = f"static_cast<{p_type}>({default})"

            decl = f"{decl} = {default}"

        return f"{decl};"

    declarations = [declare_parameter(p) for p in self.parameters]

    return '\n'.join(declarations)


def _function_forward_parameters(self):
    def translate(p, i):
        args = dotdict({
            'f': self,
            'p': p,
            'i': i
        })

        return TT.input(p.type, args).format(inp=p.name, interm=p.name_interm)

    translate_parameters = []
    has_default_parameters = False

    for i, p in enumerate(self.parameters):
        translate_parameters.append(translate(p, i))

        if p.default_argument is not None:
            has_default_parameters = True

    translate_parameters = '\n\n'.join(translate_parameters)

    if has_default_parameters:
        return code(
            """
            do {{
              {translate_parameters}
            }} while(false);
            """,
            translate_parameters=translate_parameters)
    else:
        return translate_parameters


def _function_forward_call(self):
    def forward_parameter(p):
        fwd = f"{p.name_interm}"

        if p.type.is_record() or p.type.is_lvalue_reference():
            fwd = f"*{fwd}"
        elif p.type.is_rvalue_reference():
            fwd = f"std::move(*{fwd})"

        return fwd

    parameters = ', '.join(map(forward_parameter, self.non_self_parameters))

    if self.is_instance():
        this = self.self_parameter.name_interm

    if self.is_constructor():
        call = f"new {self.parent.type}({parameters})"
    elif self.is_destructor():
        call = f"delete {this}"
    elif self.is_instance():
        call = f"{this}->{self.name.format(quals=Id.REMOVE_QUALS)}({parameters})"
    else:
        call = f"{self.name}({parameters})"

    if self.return_type.is_lvalue_reference():
        call = f"&{call}"

    if  self.return_type.is_void():
        call = f"{call};";
        outp = None
    else:
        call = f"auto {Id.RET} = {call};";
        outp = f"{Id.RET}"

    args = dotdict({
        'f': self
    })

    return_value = TT.output(self.return_type, args).format(outp=outp)

    return '\n\n'.join((call, return_value))


@property
def _parameter_name_interm(self):
    return f"_{self.name}"


Function.declare_parameters = _function_declare_parameters
Function.forward_parameters = _function_forward_parameters
Function.forward_call = _function_forward_call

Parameter.name_interm = _parameter_name_interm
