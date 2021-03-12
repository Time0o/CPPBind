import os

from file import Path


def path():
    return Path(os.path.join('cppbind', 'c', 'c_util.h'))
