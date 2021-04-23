from cppbind import Constant, Function
from patch import Patcher
from text import code
from util import dotdict


def _constant_assign(self):
    return code(
        f"""
        lua_pushstring(L, "{self.name_target()}");
        {self.type().output(outp=self.name())}
        lua_settable(L, -3);
        """)


def _function_before_call(self):
    for p in self.parameters():
        if p.default_argument() is not None:
            return "function_call:"


def _function_declare_return_value(self):
    return


def _function_perform_return(self):
    ref_returns = []

    for p in self.parameters():
        t = p.type()

        if t.is_pointer():
            t = t.pointee()
        elif t.is_reference():
            t = t.referenced()
        else:
            continue

        if not (t.is_fundamental() or t.is_enum()):
            continue

        if t.is_const():
            continue

        args = dotdict({ 'f': self })

        ref_returns.append(t.output(outp=f"*{p.name_interm()}", args=args))

    num_returns = len(ref_returns)
    if not self.return_type().is_void():
        num_returns += 1

    if not ref_returns:
        return f"return {num_returns};"

    return code(
        """
        {ref_returns}

        return {num_returns};
        """,
        num_returns=num_returns,
        ref_returns='\n'.join(ref_returns))


def _function_handle_exception(self, what):
    return f"return luaL_error(L, {what});"


class LuaPatcher(Patcher):
    def patch(self):
        self._constant_assign_orig = Constant.assign
        self._function_before_call_orig = Function.before_call
        self._function_declare_return_value_orig = Function.declare_return_value
        self._function_perform_return_orig = Function.perform_return
        self._function_handle_exception_orig = Function.handle_exception

        Constant.assign = _constant_assign
        Function.before_call = _function_before_call
        Function.declare_return_value = _function_declare_return_value
        Function.perform_return = _function_perform_return
        Function.handle_exception = _function_handle_exception

    def unpatch(self):
        Constant.assign = self._constant_assign_orig
        Function.before_call = self._function_before_call_orig
        Function.declare_return_value = self._function_declare_return_value_orig
        Function.perform_return = self_function_perform_return_orig
        Function.handle_exception = self._function_handle_exception_orig
