from abc import abstractmethod
from backend import backend
from cppbind import Enum, EnumConstant, Function, Identifier as Id, Macro, Options, Parameter, Record, Type, Variable
from text import code
from util import dotdict


def _name(get=lambda self: self.name(),
          default_namespace=None,
          default_case=None,
          default_quals=None,
          default_prefix=None,
          default_postfix=None):

    def name_closure(self,
                     namespace=default_namespace,
                     case=default_case,
                     quals=default_quals,
                     prefix=default_prefix,
                     postfix=default_postfix):

        if namespace is None:
            namespace = _name.fallback_namespace()

        if case is None:
            case = _name.fallback_case()

        if quals is None:
            quals = _name.fallback_quals()

        name = get(self)

        if namespace == 'remove' and self.namespace() is not None:
            name = name.unqualified(remove=len(self.namespace().components()))

        name = name.format(case=case, quals=quals)

        if prefix is not None:
            name = prefix + name

        if postfix is not None:
            name = name + postfix

        while name in _name.reserved():
            name += '_'

        return name

    return name_closure


def _type_target(self, args=None):
    return backend().type_translator().target(self, args)


def _type_input(self, inp, interm=None, args=None):
    return backend().type_translator().input(self, args).format(inp=inp, interm=interm)


def _type_output(self, outp, interm=None, args=None):
    return backend().type_translator().output(self, args).format(outp=outp, interm=interm)


def _type_helper(self, args=None):
    return backend().type_translator().helper(self, args)


def _function_check_parameters(self):
    pass


def _function_declare_parameters(self):
    if not self.parameters:
        return

    namespace = self.namespace()

    if namespace is None:
        using = []
    else:
        using = [f"using namespace {ns};" for ns in namespace.components()]

    def declare_parameter(p):
        t_orig = p.type()
        t = p.type()

        if t.is_record():
            t = t.with_const().pointer_to()
        elif t.is_reference():
            t = t.referenced().pointer_to()

        if t.is_const():
            t = decl_type.without_const()

        decl = f"{t} {p.name_interm()}"

        if p.default_argument() is not None:
            default = f"{p.default_argument()}"

            if t_orig.is_record() or \
               t_orig.is_reference() and t_orig.referenced().is_record():
                decl = code(
                    f"""
                    {t.pointee()} {p.name_interm()}_default = {default};
                    {decl} = &{p.name_interm()}_default;
                    """)
            else:
                decl = f"{decl} = {default};"
        else:
            decl = f"{decl};"

        return decl

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
    elif self.is_getter():
        call = self.get()
    elif self.is_setter():
        call = self.set(parameters)
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
        this = self.self().name_interm()

    custom_action = self.custom_action()

    if custom_action is not None:
        if self.is_instance():
            if parameters:
                parameters = f"{this}, {parameters}"
            else:
                parameters = f"{this}"

        call = custom_action
    else:
        if self.is_instance():
            call = f"{this}->{self.name().format(quals=Id.REMOVE_QUALS)}"
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
    return f"delete {self.self().name_interm()};"


def _function_get(self):
    p = self.property_for()

    return f"{p.name()};"


def _function_set(self, val):
    p = self.property_for()

    return f"{p.name()} = {val};"


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

    if self.can_throw():
        forward = self.try_catch(forward)

    return forward


def _function_can_throw(self):
    return not self.is_noexcept()


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
        """,
        what=what,
        std_except=self.handle_exception("__e.what()"),
        unknown_except=self.handle_exception('"exception"'))


def _function_handle_exception(self, what):
    return f"throw std::runtime_error({what});"


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

        Patcher._patch_global_actions()
        Patcher._patch_global_names()

        Patcher._init = True

    @staticmethod
    def _patch_global_actions():
        Type.target = _type_target
        Type.input = _type_input
        Type.output = _type_output
        Type.helper = _type_helper

        Function.check_parameters = _function_check_parameters
        Function.declare_parameters = _function_declare_parameters
        Function.forward_parameters = _function_forward_parameters
        Function.call = _function_call
        Function.construct = _function_construct
        Function.copy = _function_copy
        Function.move = _function_move
        Function.destruct = _function_destruct
        Function.get = _function_get
        Function.set = _function_set
        Function.before_call = _function_before_call
        Function.after_call = _function_after_call
        Function.forward_call = _function_forward_call
        Function.declare_return_value = _function_declare_return_value
        Function.forward_return_value = _function_forward_return_value
        Function.perform_return = _function_perform_return
        Function.forward = _function_forward
        Function.can_throw = _function_can_throw
        Function.try_catch = _function_try_catch
        Function.handle_exception = _function_handle_exception

    @staticmethod
    def _patch_global_names():
        _name.reserved = lambda: set()
        _name.fallback_namespace = lambda: 'keep'
        _name.fallback_case = lambda: Id.SNAKE_CASE
        _name.fallback_quals = lambda: Id.REPLACE_QUALS

        Macro.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Type.name_target = \
            _name(get=lambda t: t.name(with_template_postfix=True),
                  default_case=Id.PASCAL_CASE)

        Enum.name_target = _name(default_case=Id.PASCAL_CASE)
        EnumConstant.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Variable.name_target = _name(default_case=Id.SNAKE_CASE_CAP_ALL)

        Function.name_target = \
            _name(get=lambda f: f.name(with_template_postfix=True,
                                       with_overload_postfix=True,
                                       without_operator_name=True))

        Parameter.name_target = _name()
        Parameter.name_interm = _name(default_prefix='_')

        Record.name_target = \
          _name(get=lambda r: r.name(with_template_postfix=True),
                default_case=Id.PASCAL_CASE)

    def _patch(self, cls, attr, fn):
        self._orig[f'{cls.__name__}.{fn.__name__}'] = (cls, attr, getattr(cls, attr, None))

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
