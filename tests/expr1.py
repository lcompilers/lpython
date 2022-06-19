def test_namedexpr():
    a: i32
    x: i32
    y: i32

    # check 1
    x = (y := 0)

    # check 2
    if a := ord('3'):
        x = 1

    # check 3
    while a := 1:
        y = 1

test_namedexpr()
