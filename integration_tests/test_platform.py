from platform import python_implementation

def test_platform1():
    s: str
    s = python_implementation()
    print("Python Implementation:", s)
    assert s == "CPython" or s == "LPython"

test_platform1()
