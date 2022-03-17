from statistics import mean, fmean

def test_mean():
    eps: f64
    eps = 1e-12
    assert abs(mean([2.0]) - 2) < eps
    assert abs(mean([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]) - 4.5) < eps
    assert abs(fmean([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]) - 4.5) < eps

test_mean()
