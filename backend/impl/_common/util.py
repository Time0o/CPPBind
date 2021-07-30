# Dictionary whose members are accessible via the dot operator.
class dotdict(dict):
    __getattr__ = dict.get
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__


# Check if an object is iterable.
def is_iterable(obj):
    try:
        iter(obj)
        return True
    except:
        return False
