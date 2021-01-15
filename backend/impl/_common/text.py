import itertools
import os
import re
import textwrap


def code(txt, **kwargs):
    txt = textwrap.dedent(txt).strip()

    if kwargs:
        txt = _format(txt, kwargs)

    return txt + os.linesep


def include(header, system=False):
    return "#include " + (f"<{header}>" if system else f'"{header}"')


def compress(txt):
    txt_lines = [l.rstrip() for l in txt.strip().split('\n')]

    group_if = lambda xs, cond: ([k] if cond(k) else list(g)
                                 for k, g in itertools.groupby(xs))

    flatten = lambda xs: [x for xs_sub in xs for x in xs_sub]

    compressed = group_if(txt_lines, cond=lambda k: not k)

    return '\n'.join(flatten(compressed))


def _format(txt, kwargs):
    txt_lines = txt.split('\n')

    _format_process_kwargs(txt_lines, kwargs)

    return '\n'.join(txt_lines).format(**kwargs)


def _format_process_kwargs(txt_lines, kwargs):
    for kw, arg in kwargs.items():
        if isinstance(arg, str) and arg.count(os.linesep) > 0:
            kwargs[kw] = _format_multiline_arg(txt_lines, kw, arg)
        elif arg is None:
            kwargs[kw] = _format_none_arg(txt_lines, kw, arg)


def _format_multiline_arg(txt_lines, kw, arg):
    # XXX multiple occurrences

    ind = None
    for l in txt_lines:
        m = re.match(f'( *){{{kw}}}', l)

        if m is not None:
            ind = m[1]
            break

    if ind is None:
        return

    arg_line_first, arg_lines_rest = arg.split(os.linesep, maxsplit=1)

    return '\n'.join([arg_line_first, textwrap.indent(arg_lines_rest, ind)])


def _format_none_arg(txt_lines, kw, arg):
    txt_lines[:] = [l.replace(f'{{{kw}}}', '') for l in txt_lines]

    return arg
