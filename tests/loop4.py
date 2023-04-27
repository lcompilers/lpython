def test_for():
    i: i32
    for i in range(0, 10):
        if i == 0:
            continue
        if i > 5:
            break
        if i == 3:
            quit()

test_for()
