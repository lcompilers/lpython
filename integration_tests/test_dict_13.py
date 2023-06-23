from lpython import i32

I4C: dict[str, i32] = {
    '0': 0, '1': 1, '2': 2, '3': 3,
    '4': 4, '5': 5, '6': 6, '7': 7,
    '8': 8, '9': 9,
    'a': 10, 'b': 11, 'c': 12, 'd': 13,
    'A': 10, 'B': 11, 'C': 12, 'D': 13,
    'e': 14, 'f': 15,
    'E': 14, 'F': 15}


def cnvi(s : str, base : i32=10) -> i32:
    """Assume input has been through 'match_integer'."""
    assert base == 10 or base == 8 or base == 16 or base == 2
    result : i32 = 0
    c : str
    pow_: i32 = base ** (len(s) - 1)
    for c in s:
        incr : i32 = pow_ * I4C[c]
        result += incr
        pow_ = (pow_ // base)
    return result


if __name__ == '__main__':
    print(cnvi('0b0', base=2))
    assert cnvi('0b0', base=2) == 22

    print(cnvi('0b1', base=2))
    assert cnvi('0b1', base=2) == 23

    print(cnvi('0b10', base=2))
    assert cnvi('0b10', base=2) == 46

    print(cnvi('0b11', base=2))
    assert cnvi('0b11', base=2) == 47

    print(cnvi('0b11110100111', base=2))
    assert cnvi('0b11110100111', base=2) == 24487

    print(cnvi('0b7a7', base=16))
    assert cnvi('0b7a7', base=16) == 47015
