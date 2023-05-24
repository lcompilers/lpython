from lpython import i32, f64

def test_dict():
    rollnumber2cpi: dict[i32, f64] = {}
    i: i32
    size: i32 = 1000

    rollnumber2cpi[0] = 1.1

    for i in range(1000, 1000 + size):
        rollnumber2cpi[i] = float(i)/100.0 + 5.0

    for i in range(1000 + size - 1, 1001, -1):
        assert abs(rollnumber2cpi[i] - f64(i)/100.0 - 5.0) <= 1e-12

    assert abs(rollnumber2cpi[0] - 1.1) <= 1e-12
    assert len(rollnumber2cpi) == 1001

def test_issue_1839():
    assert len({1: 2, 1: 3, 4: 5}) == 2
    x: dict[i32, i32] = {}
    x = {1: 1, 1: 2, 1: 3}
    assert len(x) == 1


test_dict()
test_issue_1839()
