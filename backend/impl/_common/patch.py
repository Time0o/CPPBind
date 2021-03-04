from cppbind import Constant, Function, Parameter, Record, Identifier as Id
from text import code
from type_translator import TypeTranslator as TT
from util import dotdict


def _function_declare_parameters(self):
    if not self.parameters:
        return

    def declare_parameter(p):
        decl_type = p.type()

        if decl_type.is_record():
            decl_type = decl_type.pointer_to()
        elif decl_type.is_reference():
            decl_type = decl_type.referenced().pointer_to()

        if decl_type.is_const():
            decl_type = decl_type.without_const()

        decl = f"{decl_type} {p.name_interm()}"

        if p.default_argument() is not None:
            default = f"{p.default_argument()}"

            if p.type().is_scoped_enum():
                default = f"static_cast<{p.type()}>({default})"

            decl = f"{decl} = {default}"

        return f"{decl};"

    declarations = [declare_parameter(p) for p in self.parameters()]

    return '\n'.join(declarations)


def _function_forward_parameters(self):
    def translate(p, i):
        args = dotdict({
            'f': self,
            'p': p,
            'i': i
        })

        return TT().input(p.type(), args).format(inp=p.name(),
                                                 interm=p.name_interm())

    translate_parameters = []
    has_default_parameters = False

    for i, p in enumerate(self.parameters()):
        translate_parameters.append(translate(p, i))

        if p.default_argument() is not None:
            has_default_parameters = True

    translate_parameters = '\n'.join(translate_parameters)

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
        fwd = f"{p.name_interm()}"

        if p.type().is_record() or p.type().is_lvalue_reference():
            fwd = f"*{fwd}"
        elif p.type().is_rvalue_reference():
            fwd = f"std::move(*{fwd})"

        return fwd

    parameters = self.parameters()

    if self.is_instance():
        this = parameters[0].name_interm()
        parameters = parameters[1:]

    parameters = ', '.join(map(forward_parameter, parameters))

    if self.is_constructor():
        call = f"new {self.return_type().pointee()}({parameters})"
    elif self.is_destructor():
        call = f"delete {this}"
    else:
        function_name = self.name()

        if self.is_instance():
            call = f"{this}->{self.name().format(quals=Id.REMOVE_QUALS)}"
        else:
            call = f"{self.name()}"

        if self.template_argument_list():
            call = f"{call}{self.template_argument_list()}"

        call = f"{call}({parameters})"

    if self.return_type().is_lvalue_reference():
        call = f"&{call}"

    if  self.return_type().is_void():
        call = f"{call};";
        outp = None
    else:
        call = f"auto {Id.RET} = {call};";
        outp = f"{Id.RET}"

    args = dotdict({
        'f': self
    })

    return_value = TT().output(self.return_type(), args).format(outp=outp)

    return '\n\n'.join((call, return_value))


Function.declare_parameters = _function_declare_parameters
Function.forward_parameters = _function_forward_parameters
Function.forward_call = _function_forward_call


def name(get=lambda self: self.name(),
         default_case=Id.SNAKE_CASE,
         default_prefix=None,
         default_postfix=None):

    def _name_closure(self,
                      case=default_case,
                      qualified=True,
                      prefix=default_prefix,
                      postfix=default_postfix):

        quals = Id.REPLACE_QUALS if qualified else Id.REMOVE_QUALS

        name = get(self).format(case=case, quals=quals)

        if prefix is not None:
            name = prefix + name

        if postfix is not None:
            name = name + postfix

        return name

    return _name_closure


Constant.name_target = name(default_case=Id.SNAKE_CASE_CAP_ALL)

Function.name_target = name(get=lambda f: f.name(overloaded=True,
                                                 without_operator_name=True,
                                                 with_template_postfix=True))

Parameter.name_target = name()
Parameter.name_interm = name(default_prefix='_')

Record.name_target = name(get=lambda r: r.name(with_template_postfix=True),
                          default_case=Id.PASCAL_CASE)
