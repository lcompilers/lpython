from lpython import i32, f64
import random

def test_seed():
    """test the distribution of random"""
    num_samples:i32 = 100000
    bins: list[i32] = [0]*10
    _ : i32
    for _ in range(num_samples):
        val: f64 = random.random()
        assert val >= 0.0 and val < 1.0  # value out of range
        bins[i32(val * 10.0)] += 1  # increment the appropriate bin

    # Check that no bin has significantly more or fewer values than expected
    expected_bin_count:i32 = i32(num_samples / 10)
    count : i32
    for count in bins:
        blas: f64 = f64(abs(count - expected_bin_count))
        assert blas < f64(expected_bin_count) * 0.05  # allow 5% deviation

test_seed()
