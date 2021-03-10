import os
import type_info as ti

from file import Path
from text import code


def path(impl=False):
    return Path(os.path.join('cppbind', 'c', 'c_util.inc' if impl else 'c_util.h'))
