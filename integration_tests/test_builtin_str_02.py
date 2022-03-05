def _lpython_str_equal(a: str, b: str) -> bool:
    if len(a) != len(b):
        return False
    i: i32
    for i in range(len(a)):
        if a[i] != b[i]:
            return False
    return True

def f():
    assert _lpython_str_equal("a", "a")
    assert not _lpython_str_equal("a2", "a")
    assert not _lpython_str_equal("a", "a123")
    assert not _lpython_str_equal("a23", "a24")
    assert _lpython_str_equal("a24", "a24")
    assert _lpython_str_equal("abcdefg", "abcdefg")
    assert not _lpython_str_equal("abcdef3", "abcdefg")

f()
