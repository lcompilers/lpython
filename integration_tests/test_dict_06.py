from lpython import f64, i32

def fill_rollnumber2cpi(size: i32) -> dict[i32, f64]:
    i : i32
    rollnumber2cpi: dict[i32, f64] = {}
    
    rollnumber2cpi[0] = 1.1
    for i in range(1000, 1000 + size):
        rollnumber2cpi[i] = float(i)/100.0 + 5.0

    return rollnumber2cpi

def test_dict():
    i: i32
    size: i32 = 200
    rollnumber2cpi: dict[i32, f64] = fill_rollnumber2cpi(size)

    for i in range(1000 + size - 1, 1001, -1):
        assert abs(rollnumber2cpi[i] - f64(i)/100.0 - 5.0) <= 1e-12

    assert abs(rollnumber2cpi[0] - 1.1) <= 1e-12
    assert len(rollnumber2cpi) == 201

test_dict()
