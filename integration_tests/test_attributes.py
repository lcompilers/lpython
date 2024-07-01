def test_attributes() -> None:
    a: i32 = 10
    assert a.bit_length() == 4

    b: str = 'abc'
    assert b.upper() == 'ABC'

    c: list[i32] = [10, 20, 30]
    assert c[0].bit_length() == 4
    assert c.index(10) == 0

test_attributes()
