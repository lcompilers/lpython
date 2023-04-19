def f():
    x: list[f64] = [35.9, 24.9, 23.1, 4223.5, 64.34, 3095.34]
    x = sorted(x)
    ans: list[f64] = [23.1, 24.9, 35.9, 64.34, 3095.34, 4223.5]
    eps: f64 = 1e-12
    e: f64
    i: i32
    for i in range(len(x)):
        assert abs(x[i] - ans[i]) < eps
    y: list[f64] = []
    for i in range(100, -1, -1):
        y.append(float(i))
    y = sorted(y)
    for i in range(0, 101):
        assert abs(y[i] - float(i)) < eps


f()
