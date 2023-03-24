from lpython import i32

def _lpython_strcmp_eq(a: str, b: str) -> bool:
    if len(a) != len(b):
        return False
    i: i32
    for i in range(len(a)):
        if a[i] != b[i]:
            return False
    return True

def _lpython_strcmp_noteq(a: str, b: str) -> bool:
    return not _lpython_strcmp_eq(a, b)

def _lpython_strcmp_lt(a: str, b: str) -> bool:
    l: i32
    l = len(a)
    if l > len(b):
        l = len(b)
    i: i32
    for i in range(l):
        if a[i] > b[i]:
            return False
    return True

def _lpython_strcmp_lteq(a: str, b: str) -> bool:
    return _lpython_strcmp_eq(a, b) or _lpython_strcmp_lt(a, b)

def _lpython_strcmp_gt(a: str, b: str) -> bool:
    l: i32
    l = len(a)
    if l > len(b):
        l = len(b)
    i: i32
    for i in range(l):
        if a[i] < b[i]:
            return False
    return True

def _lpython_strcmp_gteq(a: str, b: str) -> bool:
    return _lpython_strcmp_eq(a, b) or _lpython_strcmp_gt(a, b)

def _lpython_str_repeat(a: str, n: i32) -> str:
    s: str
    s = ""
    i: i32
    for i in range(n):
        s += a
    return s

def f():
    assert _lpython_strcmp_eq("a", "a")
    assert not _lpython_strcmp_eq("a2", "a")
    assert not _lpython_strcmp_eq("a", "a123")
    assert not _lpython_strcmp_eq("a23", "a24")
    assert _lpython_strcmp_eq("a24", "a24")
    assert _lpython_strcmp_eq("abcdefg", "abcdefg")
    assert not _lpython_strcmp_eq("abcdef3", "abcdefg")

    assert _lpython_strcmp_noteq("a2", "a")
    assert _lpython_strcmp_noteq("a", "a123")
    assert _lpython_strcmp_noteq("a23", "a24")
    assert _lpython_strcmp_noteq("abcdef3", "abcdefg")
    assert not _lpython_strcmp_noteq("a24", "a24")

    assert _lpython_strcmp_lt("a", "a2")
    assert _lpython_strcmp_lt("a", "a123")
    assert _lpython_strcmp_lt("a23", "a24")
    assert _lpython_strcmp_lt("abcdef3", "gbc")
    assert not _lpython_strcmp_lt("bg", "abc")
    assert not _lpython_strcmp_lt("abcdefg", "abcdef3")

    assert _lpython_strcmp_lteq("a", "a2")
    assert _lpython_strcmp_lteq("a123", "a123")
    assert _lpython_strcmp_lteq("a23", "a24")
    assert not _lpython_strcmp_lteq("abcdefg", "abcdef3")

    assert _lpython_strcmp_gt("a2", "a")
    assert _lpython_strcmp_gt("a123", "a")
    assert _lpython_strcmp_gt("a24", "a23")
    assert _lpython_strcmp_gt("z", "abcdef3")
    assert not _lpython_strcmp_gt("abcdef3", "abcdefg")

    a: str
    a = "abcdefg"
    b: str
    b = "abcdef3"
    assert _lpython_strcmp_gteq("a2", "a")
    assert _lpython_strcmp_gteq("a123", "a123")
    assert _lpython_strcmp_gteq("bg", "abc")
    assert _lpython_strcmp_gteq(a, b)

    assert _lpython_str_repeat("abc", 3) == "abcabcabc"
    assert _lpython_str_repeat("", -1) == ""

f()
