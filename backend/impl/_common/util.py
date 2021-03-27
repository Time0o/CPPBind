from abc import ABCMeta


class Generic(ABCMeta):
    def __init__(cls, name, bases, clsdict):
        mro = cls.mro()

        if len(mro) >= 3:
            cls.__impl = None

            sup = mro[-2]
            sup.__impl = cls
            sup.__inst = None

        super().__init__(name, bases, clsdict)

    def __call__(cls, *args, **kwargs):
        if cls.__impl is None:
            return super().__call__(*args, **kwargs)

        if cls.__inst is None:
            cls.__inst = cls.__impl(*args, **kwargs)

        return cls.__inst


class dotdict(dict):
    __getattr__ = dict.get
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__


def is_iterable(obj):
    try:
        iter(obj)
        return True
    except:
        return False
