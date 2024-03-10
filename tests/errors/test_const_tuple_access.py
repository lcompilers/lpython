from lpython import str, tuple, Const


def test_const_tuple_access():
    CONST_TUPLE: Const[tuple[str, str, str]] = ("hello", "LPython", "!")


test_const_tuple_access()

