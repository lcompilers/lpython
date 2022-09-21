from ltypes import i32, f64

def test_dict():
    rollnumber2cpi: dict[i32, f64] = {}
    i: i32
    start: i32 = -500
    end: i32 = 501

    for i in range(start, end):
        rollnumber2cpi[i] = float(i/100.0 + 5.0)

    # for i in range(end - 1, start + 1, -1):
    #     assert abs(rollnumber2cpi[i] - i/100.0 - 5.0) <= 1e-12

    # assert len(rollnumber2cpi) == end - start + 1

test_dict()
