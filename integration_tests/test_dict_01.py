from ltypes import i32, f64

def test_dict():
    rollnumber2cpi: dict[i32, f64] = {0: 1.1}
    i: i32
    size: i32 = 1000

    for i in range(1000, 1000 + size):
        rollnumber2cpi[i] = float(i/100.0 + 5.0)

    for i in range(1000 + size - 1, 1001, -1):
        assert abs(rollnumber2cpi[i] - i/100.0 - 5.0) <= 1e-12

    assert abs(rollnumber2cpi[0] - 1.1) <= 1e-12

test_dict()
