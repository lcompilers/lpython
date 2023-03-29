from lpython import i32

def main0():
    a: i32 = 1
    b: str = "abc"
    c: list[i32] = [1, 2]
    d: list[i32] = [3, 4]
    e: tuple[i32, i32, i32] = (1, 2, 3)
    f: tuple[i32, i32] = (4, 5)
    print(a, b, c)
    print(a, c, b)
    print(c, a, b)
    print(a, b, c, sep = "pqr")
    print(a, c, b, sep = "pqr")
    print(c, a, b, sep = "pqr")
    print(a, b, c, end = "xyz\n")
    print(a, c, b, end = "xyz\n")
    print(c, a, b, end = "xyz\n")
    print(a, b, c, sep = "pqr", end = "xyz\n")
    print(a, c, b, sep = "pqr", end = "xyz\n")
    print(c, a, b, sep = "pqr", end = "xyz\n")
    # Tring out few cases with Lists and Tuples together
    print(c, e)
    print(c, d, a, sep = "pqr")
    print(c, e, d, end = "xyz\n")
    print(c, e, d, f, sep = "pqr", end = "xyz\n")

main0()