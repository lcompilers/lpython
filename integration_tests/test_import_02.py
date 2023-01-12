from test_import.import_module_01 import multiply, e
from test_import.import_module_02 import add, μ


def f():
    print(e)
    print(μ)
    print(add(10, 20))
    print(multiply(10, 20))
    assert add(10, 20) == 30
    assert multiply(10, 20) == 200

f()
