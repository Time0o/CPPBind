from cppbind import Constant, Function
from patcher import Patcher
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
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(Function, 'before_call', _function_before_call)
        self._patch(Function, 'declare_return_value', _function_declare_return_value)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'handle_exception', _function_handle_exception)
