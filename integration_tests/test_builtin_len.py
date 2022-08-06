from ltypes import i32, f64

def test_len():
    s: str
    s = "abcd"
    assert len(s) == 4
    s = ''
    assert len(s) == 0
    assert len("abcd") == 4
    assert len("") == 0

    l: list[i32]
    l = [1, 2, 3, 4]
    assert len(l) == 4
    l2: list[f64]
    l2 = [1.2, 3.4, 5.6, 7.8, 9.0]
    assert len(l2) == 5

    l3: list[i32] = []
    l3.append(1)
    l3.append(2)
    l3.append(3)
    assert len(l3) == 3

test_len()
