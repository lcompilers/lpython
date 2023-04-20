from numpy import array

def any_01() -> None:
    x: bool[2]

    x= array([False, False])
    assert not any(x)

    x = array([False, True])
    assert any(x)

    x = array([True, True])
    assert any(x)

any_01()
