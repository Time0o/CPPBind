from cppbind import Function, Identifier as Id, Record
from patcher import Patcher, _name
from text import code
from util import dotdict


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
    extra_returns = []

    for i, p in enumerate(self.parameters()):
        t_orig = p.type()
        t = t_orig

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

        ret = t.output(outp=f"*{p.name_interm()}", args=args)

        if t_orig.is_pointer():
            ret = code(
                """
                if ({guard})
                   {ret}
                """,
                guard=f'!lua_isuserdata(L, {i+1}) && {p.name_interm()}',
                ret=ret)

        extra_returns.append(ret)

    num_returns = 0 if self.return_type().is_void() else 1

    if not extra_returns:
        return f"return {num_returns};"

    return code(
        """
        int __top = lua_gettop(L);

        {extra_returns}

        return {num_returns} + (lua_gettop(L) - __top);
        """,
        extra_returns='\n'.join(extra_returns),
        num_returns=num_returns)


def _function_can_throw(self):
    if not self.is_noexcept():
        return True

    for p in self.parameters():
        if p.is_self():
            continue

        t = p.type()

        if t.is_record() or \
           t.is_pointer() or \
           (t.is_reference() and not t.referenced().is_fundamental()):
            return True

    return False


def _function_try_catch(self, what):
    return code(
        """
        try {{
          {what}
        }} catch (std::exception const &__e) {{
          lua_pushstring(L, __e.what());
        }} catch (lua_longjmp *) {{
          throw;
        }} catch (...) {{
          lua_pushstring(L, "exception");
        }}

        return lua_error(L);
        """,
        what=what)



class LuaPatcher(Patcher):
    def patch(self):
        self._patch(Function, 'check_parameters', _function_check_parameters)
        self._patch(Function, 'before_call', _function_before_call)
        self._patch(Function, 'declare_return_value', _function_declare_return_value)
        self._patch(Function, 'perform_return', _function_perform_return)
        self._patch(Function, 'can_throw', _function_can_throw)
        self._patch(Function, 'try_catch', _function_try_catch)
