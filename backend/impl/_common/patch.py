from abc import abstractmethod
from backend import backend
from cppbind import Constant, Function, Identifier as Id, Options, Parameter, Record, Type
from text import code
from util import dotdict


def _name(get=lambda self: self.name(),
          default_case=Id.SNAKE_CASE,
          default_prefix=None,
          default_postfix=None):

    def name_closure(self,
                     case=default_case,
                     prefix=default_prefix,
                     postfix=default_postfix,
                     qualified=True):

        quals = Id.REPLACE_QUALS if qualified else Id.REMOVE_QUALS

        name = get(self).format(case=case, quals=quals)

        if prefix is not None:
            name = prefix + name

        if postfix is not None:
            name = name + postfix

        return name

    return name_closure


def _type_target(self, args=None):
    return backend().type_translator().target(self, args)


# XXX constant


def _type_output(self, outp, args=None):
    return backend().type_translator().output(self, args).format(outp=outp)


def _type_input(self, inp, interm, args=None):
    return backend().type_translator().input(self, args).format(inp=inp, interm=interm)


def _constant_assign(self):
    args = dotdict({
        'c': self
    })

    return backend().type_translator().constant(self.type(), args).format(const=self.name())


def _function_declare_parameters(self):
    if not self.parameters:
        return

    using_directives = [f"using namespace {ns};" for ns in self.enclosing_namespaces()]

    def declare_parameter(p):
        decl_name = p.name_interm()
        decl_type = p.type()

        if decl_type.is_record():
            decl_type = decl_type.pointer_to()
        elif decl_type.is_reference():
            decl_type = decl_type.referenced().pointer_to()

        if decl_type.is_const():
            decl_type = decl_type.without_const()

        decl = f"{decl_type} {decl_name}"

        if p.default_argument() is not None:
            default = f"{p.default_argument()}"

            if p.type().is_record() or \
               p.type().is_reference() and p.type().referenced().is_record():
                decl = code(
                    f"""
                    {decl_type.pointee()} {decl_name}_default = {default};
                    {decl} = &{decl_name}_default;
                    """)
            else:
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

        return p.type().input(inp=p.name_target(), interm=p.name_interm(), args=args)

    translate_parameters = []
    for i, p in enumerate(self.parameters()):
        translate_parameters.append(translate(p, i))

    return '\n\n'.join(translate_parameters)


def _function_forward_call(self):
    def forward_parameter(p):
        if isinstance(p, str):
            return p

        fwd = f"{p.name_interm()}"

        if p.type().is_record() or p.type().is_lvalue_reference():
            fwd = f"*{fwd}"
        elif p.type().is_rvalue_reference():
            fwd = f"std::move(*{fwd})"

        return fwd

    parameters = []

    for p in self.parameters():
        if p.is_self():
            this = p.name_interm()
        else:
            parameters.append(p)

    if self.is_overloaded_operator('++', 1):
        parameters.append('{}')

    parameters = ', '.join(map(forward_parameter, parameters))

    if self.is_constructor():
        call = self.construct(parameters)
    elif self.is_destructor():
        call = self.destruct(this)
    else:
        function_name = self.name()

        if self.is_instance():
            call = f"{this}->{self.name().format(quals=Id.REMOVE_QUALS)}"
        else:
            call = f"{self.name()}"

        if self.template_argument_list():
            call = f"{call}{self.template_argument_list()}"

        call = f"{call}({parameters});"

    if self.return_type().is_lvalue_reference():
        call = f"&{call}"

    if self.return_type().is_void():
        call = f"{call}";
        outp = None
    else:
        call = f"auto {Id.RET} = {call}";
        outp = f"{Id.RET}"

    args = dotdict({
        'f': self
    })

    call_return = self.return_type().output(outp=outp, args=args)

    call = code(
        """
        {call_before}

        {call}

        {call_after}

        {call_return}
        """,
        call_before=self.before_call(),
        call=call,
        call_after=self.after_call(),
        call_return=call_return)

    return call


def _function_before_call(self):
    return


def _function_after_call(self):
    return


def _function_construct(self, parameters):
    t = self.parent().type()

    return f"new {t}({parameters});"


def _function_destruct(self, this):
    return f"delete {this};"


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

    fmt = backend().type_translator().exception(args)
    std_except = fmt.format(what='__e.what()')
    unknown_except = fmt.format(what='"exception"')

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


class Patcher:
    _init = False

    def __init__(self):
        if Patcher._init:
            return

        Type.target = _type_target
        Type.input = _type_input
        Type.output = _type_output

        Constant.assign = _constant_assign

        Function.declare_parameters = _function_declare_parameters
        Function.forward_parameters = _function_forward_parameters
        Function.before_call = _function_before_call
        Function.after_call = _function_after_call
        Function.construct = _function_construct
        Function.destruct = _function_destruct
        Function.forward_call = _function_forward_call
        Function.forward = _function_forward
        Function.try_catch = _function_try_catch

        Constant.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Function.name_target = \
            _name(get=lambda f: f.name(with_template_postfix=True,
                                       with_overload_postfix=True,
                                       without_operator_name=True))

        Parameter.name_target = _name()
        Parameter.name_interm = _name(default_prefix='_')

        Record.name_target = \
          _name(get=lambda r: r.name(with_template_postfix=True),
                default_case=Id.PASCAL_CASE)

        Patcher._init = True

    @abstractmethod
    def patch(self):
        pass

    @abstractmethod
    def unpatch(self):
        pass
