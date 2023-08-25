def f():
    s1: str = "abcd"
    s2: str = "abcd"
    assert s1 == s2
    assert s1 <= s2
    assert s1 >= s2
    s1 = "abcde"
    assert s1 >= s2
    assert s1 > s2
    s1 = "abc"
    assert s1 < s2
    assert s1 <= s2
    s1 = "Abcd"
    s2 = "abcd"
    assert s1 < s2
    s1 = "orange"
    s2 = "apple"
    assert s1 >= s2
    assert s1 > s2
    s1 = "albatross"
    s2 = "albany"
    assert s1 >= s2
    assert s1 > s2
    assert s1 != s2
    s1 = "maple"
    s2 = "morning"
    assert s1 <= s2
    assert s1 < s2
    assert s1 != s2
    s1 = "Zebra"
    s2 = "ant"
    assert s1 <= s2
    assert s1 < s2
    assert s1 != s2

    print("Ok")

f()
