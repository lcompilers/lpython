import test_modules

def main0():
    print(test_modules.sinx(0.5))
    print(test_modules.cosx(0.5))
    assert abs(test_modules.sinx(0.5) + test_modules.cosx(0.5) - 1.0) <= 1e-12

main0()
