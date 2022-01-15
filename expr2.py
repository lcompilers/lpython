def main():
    a: bool
    a = 5 == 5 and 5 != 5
    a = 5 < 3 and 5 != 5
    a = 5 == 5 or 5 != 5

    b: i32
    b = 4
    b = 2 if b == 4 else 6
