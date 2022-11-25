import test_modules

def main0():
    print(test_modules.sin(0.5))
    print(test_modules.cos(0.5))
    assert abs(test_modules.sin(0.5) + test_modules.cos(0.5) - 1.0) <= 1e-12

main0()
