from lpython import i32, f64

def fill_rollnumber2cpi(size: i32) -> dict[i32, f64]:
    i : i32
    rollnumber2cpi: dict[i32, f64] = {}
    
    rollnumber2cpi[0] = 1.1
    for i in range(1000, 1000 + size, 7):
        rollnumber2cpi[i] = f64(i)/100.0 + 5.0

    return rollnumber2cpi

def test_dict():
    i: i32
    size: i32 = 7000
    size1: i32

    rollnumber2cpi: dict[i32, f64] = fill_rollnumber2cpi(size)

    size1 = i32(size/7 + 1.0)
    for i in range(1000, 1000 + size//2, 7):
        assert abs(rollnumber2cpi.pop(i) - f64(i)/100.0 - 5.0) <= 1e-12
        size1 = size1 - 1
        assert len(rollnumber2cpi) == size1

    for i in range(1000, 1000 + size//2, 7):
        rollnumber2cpi[i] = - f64(i)/100.0 - 5.0

    for i in range(1000, 1000 + size//2, 7):
        assert abs(rollnumber2cpi[i] + f64(i)/100.0 + 5.0) <= 1e-12

    for i in range(1000, 1000 + size, 7):
        rollnumber2cpi[i] = - f64(i)/100.0 - 5.0

    for i in range(1000, 1000 + size, 7):
        assert abs(rollnumber2cpi[i] + f64(i)/100.0 + 5.0) <= 1e-12

    assert abs(rollnumber2cpi[0] - 1.1) <= 1e-12

test_dict()
