from lpython import i32

def main():
    d: dict[str, i32] = {
        '2': 2, '3': 3,
        '4': 4, '5': 5, '6': 6, '7': 7,
        '8': 8, '9': 9,
        'a': 10, 'b': 11, 'c': 12, 'd': 13,
        'A': 100, 'B': 110, 'C': 120, 'D': 130,
        'e': 14, 'f': 15,
        'E': 140, 'F': 150}

    assert (d['2'] == 2)
    assert (d['3'] == 3)
    assert (d['4'] == 4)
    assert (d['5'] == 5)
    assert (d['6'] == 6)
    assert (d['7'] == 7)
    assert (d['8'] == 8)
    assert (d['9'] == 9)

    assert (d['a'] == 10)
    assert (d['b'] == 11)
    assert (d['c'] == 12)
    assert (d['d'] == 13)
    assert (d['A'] == 100)
    assert (d['B'] == 110)
    assert (d['C'] == 120)
    assert (d['D'] == 130)

    assert (d['e'] == 14)
    assert (d['f'] == 15)
    assert (d['E'] == 140)
    assert (d['F'] == 150)

main()
