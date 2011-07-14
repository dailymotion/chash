from ctypes import *
libchash = CDLL("libchash.so.1")

class CHASH_TARGET(Structure):
    _fields_ = [
        ('weight', c_ubyte),
        ('name', c_char_p)]

class CHASH_ITEM(Structure):
    _fields_ = [
        ('hash', c_uint, 32),
        ('target', c_uint, 16)]

class CHASH_LOOKUP(Structure):
    _fields_ = [
        ('rank', c_uint, 16),
        ('target', c_uint, 16)]

class CHASH_CONTEXT(Structure):
    _fields_ = [
        ('magic', c_uint, 32),
        ('frozen', c_ubyte),
        ('targets_count', c_uint, 16),
        ('targets', POINTER(CHASH_TARGET)),
        ('items_count', c_uint, 32),
        ('continuum', POINTER(CHASH_ITEM)),
        ('lookups', POINTER(CHASH_LOOKUP)),
        ('lookup', POINTER(c_char_p))]
    
libchash.chash_add_target.argtypes = [POINTER(CHASH_CONTEXT), c_char_p, c_ubyte]
libchash.chash_unserialize.argtypes = [POINTER(CHASH_CONTEXT), c_char_p, c_uint]
libchash.chash_remove_target.argtypes = [POINTER(CHASH_CONTEXT), c_char_p]
#libchash.chash_lookup.argtypes = [POINTER(CHASH_CONTEXT), c_char_p, c_uint, pointer(c_char_p)]
#libchash.chash_lookup_balance.argtypes = [POINTER(CHASH_CONTEXT), c_char_p, c_uint, LP_LP_c_char_p]

CHASH_ERROR_DONE = 0
CHASH_ERROR_MEMORY = -1
CHASH_ERROR_IO = -2
CHASH_ERROR_INVALID_PARAMETER = - 10
CHASH_ERROR_ALREADY_INITIALIZED = -11
CHASH_ERROR_NOT_INITIALIZED = -12
CHASH_ERROR_NOT_FOUND = -13

class CHashError(Exception): pass

def chash_return(status, zero_is_none):
    if status == 0 and zero_is_none:
        return None
    elif status >= 0:
        return status
    elif status == CHASH_ERROR_MEMORY:
        raise CHashError('No memory')
    elif status == CHASH_ERROR_NOT_FOUND:
        raise CHashError('No element found')
    elif status == CHASH_ERROR_IO:
        raise CHashError('IO error')
    elif status == CHASH_ERROR_INVALID_PARAMETER:
        raise CHashError('Invalid parameter')
    elif status == CHASH_ERROR_ALREADY_INITIALIZED:
        raise CHashError('Already initialized')
    elif status == CHASH_ERROR_NOT_INITIALIZED:
        raise CHashError('Not yet initialized')
    else:
        raise CHashError('Unknown exception')

class CHash(object):

    def __init__(self):
        self._ctx = CHASH_CONTEXT()
        libchash.chash_initialize(byref(self._ctx))
    
    def __del__(self):
        libchash.chash_terminate(byref(self._ctx))

    def add_target(self, target, weight=1):
        res = libchash.chash_add_target(byref(self._ctx), target, weight)
        return chash_return(res, True)

    def set_targets(self, targets):
        if not isinstance(targets, dict):
            raise TypeError()
        i = 0
        for target, weight in targets.items():
            try:
                status = libchash.chash_add_target(byref(self._ctx), target, weight)
            except ArgumentError, e:
                raise TypeError()
            chash_return(status, False)
            i += 1
        return chash_return(self.count_targets(), False)

    def remove_target(self, target):
        status = libchash.chash_remove_target(byref(self._ctx), target)
        return chash_return(status, True)

    def count_targets(self):
        status = libchash.chash_targets_count(byref(self._ctx))
        return chash_return(status, False)

    def clear_targets(self):
        status = libchash.chash_clear_targets(byref(self._ctx))
        return chash_return(status, True)
        
    def lookup_balance(self, candidate, count=1):
        target = c_char_p()
        status = libchash.chash_lookup_balance(byref(self._ctx), candidate, count, byref(target))
        if status < 0:
            return chash_return(status, True)
        return target.value

    def lookup_list(self, candidate, count=1):
        targets = pointer(c_char_p())
        status = libchash.chash_lookup(byref(self._ctx), candidate, count, byref(targets))
        if status <= 0:
            return chash_return(status, True)
        return [targets[i] for i in range(status)]

    def serialize(self):
        serialized = pointer(c_char())
        status = libchash.chash_serialize(byref(self._ctx), byref(serialized))
        if status < 0:
            return chash_return(status, True)
        return serialized[:status]

    def unserialize(self, serialized):
        status = libchash.chash_unserialize(byref(self._ctx), serialized, len(serialized))
        return chash_return(status, True)

    def serialize_to_file(self, path):
        status = libchash.chash_file_serialize(byref(self._ctx), path)
        return chash_return(status, True)

    def unserialize_from_file(self, path):
        status = libchash.chash_file_unserialize(byref(self._ctx), path)
        return chash_return(status, True)
