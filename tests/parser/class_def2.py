class _DTypeDict(_DTypeDictBase, total=False):
    pass

class test_01():
    pass

class test_02(x, y):
    pass

class test_03(x, y = 1, z = '123'):
    pass

class test_04(100, **x):
    pass

class test_05(*x, **y):
    pass

class test_06(*x, y):
    pass

class test_07(*x, *y):
    pass

class test_08(**x, **y):
    pass

class test_08(x): # type: ignore[misc]
    pass

class sub(np.ndarray): pass

class name: ...;

class MMatrix(MaskedArray, np.matrix,): ...
