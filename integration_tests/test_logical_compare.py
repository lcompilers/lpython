from lpython import i32, f64


def test_logical_compare_literal():
    # Integers
    print(1 or 3)
    assert (1 or 3) == 1

    print(1 and 3)
    assert (1 and 3) == 3

    print(2 or 3 or 5 or 6)
    assert (2 or 3 or 5 or 6) == 2

    print(1 and 3 or 2 and 4)
    assert (1 and 3 or 2 and 4) == 3

    print(1 or 3 and 0 or 4)
    assert (1 or 3 and 0 or 4) == 1

    print(1 and 3 or 2 and 0)
    assert (1 and 3 or 2 and 0) == 3

    print(1 and 0 or 3 and 4)
    assert (1 and 0 or 3 and 4) == 4

    # Floating-point numbers
    print(1.33 or 6.67)
    assert (1.33 or 6.67) == 1.33

    print(1.33 and 6.67)
    assert (1.33 and 6.67) == 6.67

    print(1.33 or 6.67 and 3.33 or 0.0)
    assert (1.33 or 6.67 and 3.33 or 0.0) == 1.33

    print(1.33 and 6.67 or 3.33 and 0.0)
    assert (1.33 and 6.67 or 3.33 and 0.0) == 6.67

    print(1.33 and 0.0 and 3.33 and 6.67)
    assert (1.33 and 0.0 and 3.33 and 6.67) == 0.0

    # Strings
    print("a" or "b")
    assert ("a" or "b") == "a"

    print("abc" or "b")
    assert ("abc" or "b") == "abc"

    print("a" and "b")
    assert ("a" and "b") == "b"

    print("a" or "b" and "c" or "d")
    assert ("a" or "b" and "c" or "d") == "a"

    print("" or " ")
    assert ("" or " ") == " "

    print("" and " " or "a" and "b" and "c")
    assert ("" and " " or "a" and "b" and "c") == "c"

    print("" and " " and "a" and "b" and "c")
    assert ("" and " " and "a" and "b" and "c") == ""


def test_logical_compare_variable():
    # Integers
    i_a: i32 = 1
    i_b: i32 = 3

    print(i_a and i_b)
    assert (i_a and i_b) == 3

    print(i_a or i_b or 2 or 4)
    assert (i_a or i_b or 2 or 4) == 1

    print(i_a and i_b or 2 and 4)
    assert (i_a and i_b or 2 and 4) == 3

    print(i_a or i_b and 0 or 4)
    assert (i_a or i_b and 0 or 4) == i_a

    print(i_a and i_b or 2 and 0)
    assert (i_a and i_b or 2 and 0) == i_b

    print(i_a and 0 or i_b and 4)
    assert (i_a and 0 or i_b and 4) == 4

    print(i_a + i_b or 0 - 4)
    assert (i_a + i_b or 0 - 4) == 4

    # Floating-point numbers
    f_a: f64 = 1.67
    f_b: f64 = 3.33

    print(f_a // f_b and f_a - f_b)
    assert (f_a // f_b and f_a - f_b) == 0.0

    print(f_a**3.0 or 3.0**f_a)
    assert (f_a**3.0 or 3.0**f_a) == 4.657462999999999

    print(f_a - 3.0 and f_a + 3.0 or f_b - 3.0 and f_b + 3.0)
    assert (f_a - 3.0 and f_a + 3.0 or f_b - 3.0 and f_b + 3.0) == 4.67

    # Can be uncommented after fixing the segfault
    # Strings
    # s_a: str = "a"
    # s_b: str = "b"

    # print(s_a or s_b)
    # assert (s_a or s_b) == s_a

    # print(s_a and s_b)
    # assert (s_a and s_b) == s_b

    # print(s_a + s_b or s_b + s_a)
    # assert (s_a + s_b or s_b + s_a) == "ab"

    # print(s_a[0] or s_b[-1])
    # assert (s_a[0] or s_b[-1]) == "a"

    # print(s_a[0] and s_b[-1])
    # assert (s_a[0] and s_b[-1]) == "b"

    # print(s_a + s_b or s_b + s_a + s_a[0] and s_b[-1])
    # assert (s_a + s_b or s_b + s_a + s_a[0] and s_b[-1]) == "ab"


test_logical_compare_literal()
test_logical_compare_variable()
