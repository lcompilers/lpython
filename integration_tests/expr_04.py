def main0():
    i: i32
    sum: i32
    sum = 0
    for i in range(0, 10):
        if i == 0:
            continue
        sum += i
        if i > 5:
            break
    assert sum == 21


def test_issue_255():
    i: f64
    i = 300.27
    eps: f64
    eps = 1e-12
    assert abs(i//1.0 - 300.0) < eps


test_issue_255()
main0()
