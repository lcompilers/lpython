from lpython import f64

# test issue 1671
def test_fast_fma() -> f64:
    a : f64 = 5.00
    a = a + a * 10.00
    return a

print(test_fast_fma())
