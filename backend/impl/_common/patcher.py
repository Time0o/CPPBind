from abc import abstractmethod
from backend import backend
from cppbind import Constant, Definition, Enum, EnumConstant, Function, Identifier as Id, Options, Parameter, Record, Type
from text import code
from util import dotdict


def _name(get=lambda self: self.name(),
          default_case=None,
          default_quals=None,
          default_prefix=None,
          default_postfix=None):

    def name_closure(self,
                     case=default_case,
                     quals=default_quals,
                     prefix=default_prefix,
                     postfix=default_postfix):

        if case is None:
            case = _name.fallback_case()

        if quals is None:
            quals = _name.fallback_quals()

        name = get(self).format(case=case, quals=quals)

        if prefix is not None:
            name = prefix + name

        if postfix is not None:
            name = name + postfix

        return name

    return name_closure


def _type_target(self, args=None):
    return backend().type_translator().target(self, args)


def _type_input(self, inp, interm=None, args=None):
    return backend().type_translator().input(self, args).format(inp=inp, interm=interm)


def _type_output(self, outp, interm=None, args=None):
    return backend().type_translator().output(self, args).format(outp=outp, interm=interm)


def _constant_declare(self):
    return f"{self.type().target()} {self.name_target()}"


def _constant_assign(self):
    args = dotdict({ 'c': self })

    return self.type().output(args=args).format(outp=self.name(), interm=self.name_target())


def _function_check_parameters(self):
    pass


def _function_declare_parameters(self):
    if not self.parameters:
        return

    ns = self.namespace()

    if ns is None:
        using = None
    else:
        using = [f"using namespace {sub_ns};" for sub_ns in ns.components()]

    def declare_parameter(p):
        decl_name = p.name_interm()
        decl_type = p.type()

        if decl_type.is_record():
            decl_type = decl_type.with_const().pointer_to()
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
        {using}

        {declarations}
        """,
        using='\n'.join(using),
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
        if not p.is_self():
            parameters.append(p)

    if self.is_overloaded_operator('++', 1):
        parameters.append('{}')

    parameters = ', '.join(map(forward_parameter, parameters))

    if self.is_constructor():
        if self.is_copy_constructor():
            call = self.copy(parameters)
        elif self.is_move_constructor():
            call = self.move(parameters)
        else:
            call = self.construct(parameters)
    elif self.is_destructor():
        call = self.destruct()
    else:
        call = self.call(parameters)

    if self.return_type().is_lvalue_reference():
        call = f"&{call}"

    if self.return_type().is_void():
        call = f"{call}";
    else:
        call = f"auto {Id.OUT} = {call}";

    call = code(
        """
        {call_before}

        {call}

        {call_after}
        """,
        call_before=self.before_call(),
        call=call,
        call_after=self.after_call())

    return call


def _function_call(self, parameters):
    if self.is_instance():
        call = f"{self.this().name_interm()}->{self.name().format(quals=Id.REMOVE_QUALS)}"
    else:
        call = f"{self.name()}"

    if self.template_argument_list():
        call = f"{call}{self.template_argument_list()}"

    return f"{call}({parameters});"


def _function_construct(self, parameters):
    t = self.parent().type()

    return f"new {t}({parameters});"


def _function_copy(self, parameters):
    return self.construct(parameters)


def _function_move(self, parameters):
    return self.construct(parameters)


def _function_destruct(self):
    return f"delete {self.this().name_interm()};"


def _function_before_call(self):
    return


def _function_after_call(self):
    return


def _function_declare_return_value(self):
    if self.return_type().is_void():
        return

    return f"{self.return_type().target()} {Id.RET};"


def _function_forward_return_value(self):
    if self.return_type().is_void():
        return

    args = dotdict({ 'f': self })

    return self.return_type().output(outp=str(Id.OUT), interm=str(Id.RET), args=args)


def _function_perform_return(self):
    return


def _function_forward(self):
    forward = code(
        """
        {check_parameters}

        {declare_parameters}

        {forward_parameters}

        {forward_call}

        {declare_return_value}

        {forward_return_value}

        {perform_return}
        """,
        check_parameters=self.check_parameters(),
        declare_parameters=self.declare_parameters(),
        forward_parameters=self.forward_parameters(),
        forward_call=self.forward_call(),
        declare_return_value=self.declare_return_value(),
        forward_return_value=self.forward_return_value(),
        perform_return=self.perform_return())

    if not self.is_noexcept():
        forward = self.try_catch(forward)

    return forward


def _function_try_catch(self, what):
    return code(
        """
        try {{
          {what}
        }} catch (std::exception const &__e) {{
          {std_except}
        }} catch (...) {{
          {unknown_except}
        }}

        {finalize_except}
        """,
        what=what,
        std_except=self.handle_exception("__e.what()"),
        unknown_except=self.handle_exception('"exception"'),
        finalize_except=self.finalize_exception())


def _function_handle_exception(self, what):
    return f"throw std::runtime_error({what});"


def _function_finalize_exception(self):
    pass


class Patcher:
    _init = False

    def __init__(self):
        if not Patcher._init:
            Patcher._patch_global()

        self._orig = {}

    @staticmethod
    def _patch_global():
        if Patcher._init:
            return

        _name.fallback_case = lambda: Id.SNAKE_CASE
        _name.fallback_quals = lambda: Id.REPLACE_QUALS

        Type.target = _type_target
        Type.input = _type_input
        Type.output = _type_output

        Constant.declare = _constant_declare
        Constant.assign = _constant_assign

        Function.check_parameters = _function_check_parameters
        Function.declare_parameters = _function_declare_parameters
        Function.forward_parameters = _function_forward_parameters
        Function.call = _function_call
        Function.construct = _function_construct
        Function.copy = _function_copy
        Function.move = _function_move
        Function.destruct = _function_destruct
        Function.before_call = _function_before_call
        Function.after_call = _function_after_call
        Function.forward_call = _function_forward_call
        Function.declare_return_value = _function_declare_return_value
        Function.forward_return_value = _function_forward_return_value
        Function.perform_return = _function_perform_return
        Function.forward = _function_forward
        Function.try_catch = _function_try_catch
        Function.handle_exception = _function_handle_exception
        Function.finalize_exception = _function_finalize_exception

        Constant.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Definition.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Enum.name_target = _name(default_case=Id.PASCAL_CASE)
        EnumConstant.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

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

    def _patch(self, cls, attr, fn):
        self._orig[fn.__name__] = (cls, attr, getattr(cls, attr, None))

        setattr(cls, attr, fn)

    @abstractmethod
    def patch(self):
        pass

    def unpatch(self):
        for cls, attr, fn in self._orig.values():
            if fn is None:
                delattr(cls, attr)
            else:
                setattr(cls, attr, fn)
