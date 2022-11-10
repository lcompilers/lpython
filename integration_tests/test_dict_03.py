from ltypes import f64, i32, i64

def power2(i: i64, mod: i64) -> i64:
    if i == i64(0) or i == i64(1):
        return i64(2)**i

    p1: i64; p2: i64; px: i64; py: i64;

    p1 = i//i64(2)
    p2 = i - p1
    px = power2(p1, mod) % mod
    py = power2(p2, mod) % mod
    return (px * py) % mod

def generate_key(i: i32) -> str:
    i2d: dict[i32, str] = {0: 'a', 1: 'b', 2: 'c', 3: 'd',
                           4: 'e', 5: 'f', 6: 'g', 7: 'h',
                           8: 'i', 9: 'j'}
    mod: i32 = 99997
    key_digits: i32
    key_digits = i32(power2(int(i), int(mod)))
    key: str = ""

    while key_digits > 0:
        key += i2d[key_digits%10]
        key_digits = key_digits//10
    return key

def test_dict():
    number2cpi: dict[str, i64] = {}
    i: i32
    size: i32 = 150
    cd: i32 = 1
    size1: i32
    key: str

    size = cd * size

    for i in range(1000, 1000 + size, 1):
        key = generate_key(i)
        number2cpi[key] = power2(int(i), int(99997))

    size1 = len(number2cpi)
    for i in range(1000, 1000 + size//2, 1):
        key = generate_key(i)
        assert number2cpi.pop(key) == power2(int(i), int(99997))
        size1 = size1 - 1
        assert len(number2cpi) == size1

    for i in range(1000, 1000 + size//2, 1):
        key = generate_key(i)
        number2cpi[key] = -power2(int(i), int(99997))

    for i in range(1000, 1000 + size//2, 1):
        key = generate_key(i)
        assert number2cpi[key] == -power2(int(i), int(99997))

    for i in range(1000, 1000 + size, 1):
        key = generate_key(i)
        number2cpi[key] = -power2(int(i), int(99997))

    for i in range(1000, 1000 + size, 1):
        key = generate_key(i)
        assert number2cpi[key] == -power2(int(i), int(99997))

    for i in range(1000 + size//4, 1000 + size//2, 1):
        key = generate_key(i)
        print(key, number2cpi[key])

test_dict()
