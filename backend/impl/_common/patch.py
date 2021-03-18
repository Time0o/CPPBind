from cppbind import Constant, Function, Identifier as Id, Options, Parameter, Record, Type
from text import code
from type_translator import TypeTranslator as TT
from util import dotdict


def _function_declare_parameters(self):
    if not self.parameters:
        return

    using_directives = [f"using namespace {ns};" for ns in self.enclosing_namespaces()]

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

    return code(
        """
        {using_directives}

        {declarations}
        """,
        using_directives='\n'.join(using_directives),
        declarations='\n'.join(declarations))


def _function_forward_parameters(self):
    if not self.parameters():
        return

    def translate(p, i):
        args = dotdict({
            'f': self,
            'p': p,
            'i': i
        })

        return TT().input(p.type(), args).format(inp=p.name_target(),
                                                 interm=p.name_interm())

    translate_parameters = []
    has_default_parameters = False

    for i, p in enumerate(self.parameters()):
        translate_parameters.append(translate(p, i))

        if p.default_argument() is not None:
            has_default_parameters = True

    translate_parameters = '\n\n'.join(translate_parameters)

    if has_default_parameters:
        translate_parameters = code(
            """
            do {{
              {translate_parameters}
            }} while(false);
            """,
            translate_parameters=translate_parameters)

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

    call_return = TT().output(self.return_type(), args).format(outp=outp)

    call = code(
        """
        {call}
        {call_return}
        """,
        call=call,
        call_return=call_return)

    return call


def _function_forward(self):
    forward = code(
        """
        {declare_parameters}

        {forward_parameters}

        {forward_call}
        """,
        declare_parameters=self.declare_parameters(),
        forward_parameters=self.forward_parameters(),
        forward_call=self.forward_call())

    if not Options.output_noexcept and not self.is_noexcept():
        forward = self.try_catch(forward)

    return forward


def _function_try_catch(self, what):
    args = dotdict({
        'f': self
    })

    std_except = TT().exception(args).format(what='__e.what()')
    unknown_except = TT().exception(args).format(what='"exception"')

    return code(
        """
        try {{
          {what}
        }} catch (std::exception const &__e) {{
          {std_except}
        }} catch (...) {{
          {unknown_except}
        }}
        """,
        what=what,
        std_except=std_except,
        unknown_except=unknown_except)


Function.declare_parameters = _function_declare_parameters
Function.forward_parameters = _function_forward_parameters
Function.forward_call = _function_forward_call
Function.forward = _function_forward
Function.try_catch = _function_try_catch


def _constant_assign(self):
    args = dotdict({
        'c': self
    })

    return TT().constant(self.type(), args).format(const=self.name())

Constant.assign = _constant_assign


def _name(get=lambda self: self.name(),
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


Constant.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

Function.name_target = _name(get=lambda f: f.name(overloaded=True,
                                                  without_operator_name=True,
                                                  with_template_postfix=True))

Parameter.name_target = _name()
Parameter.name_interm = _name(default_prefix='_')

Record.name_target = _name(get=lambda r: r.name(with_template_postfix=True),
                           default_case=Id.PASCAL_CASE)


def _type_target(self):
    return TT().target(self)

Type.target = _type_target
