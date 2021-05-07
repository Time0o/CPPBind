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


def _function_check_parameters(self):
    num_min = sum(1 for p in self.parameters() if p.default_argument() is None)
    num_max = len(self.parameters())

    if num_min == num_max:
        return code(
            f"""
            if (lua_gettop(L) != {num_min})
              return luaL_error(L, "function expects {num_min} arguments");
            """)

    return code(
        f"""
        if (lua_gettop(L) < {num_min} || lua_gettop(L) > {num_max})
          return luaL_error(L, "function expects between {num_min} and {num_max} arguments");
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
    return f"lua_pushstring(L, {what});"


def _function_finalize_exception(self):
    return f"return lua_error(L);"


class LuaPatcher(Patcher):
    def patch(self):
        self._patch(Constant, 'assign', _constant_assign)
        self._patch(Function, 'check_parameters', _function_check_parameters)
        self._patch(Function, 'before_call', _function_before_call)
        self._patch(Function, 'declare_return_value', _function_declare_return_value)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'handle_exception', _function_handle_exception)
        self._patch(Function, 'finalize_exception', _function_finalize_exception)
