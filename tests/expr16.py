def test_issue_455(data: list[i32]) -> f64:
    sum: f64
    sum = 0.0
    i: i32
    for i in range(len(data)):
        sum += data[i]
    return sum
