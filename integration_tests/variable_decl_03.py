from lpython import f64

def f() -> f64:
    return 5.5

def main():
    t1: f64 = f() * 1e6
    print(t1)
    assert abs(t1 - 5.5 * 1e6) <= 1e-6

main()
