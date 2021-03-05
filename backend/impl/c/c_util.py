import type_info as ti

from text import code


def include(impl=False):
    if impl:
        return '#include "cppbind/c/c_util.inc"'
    else:
        return '#include "cppbind/c/c_util.h"'
