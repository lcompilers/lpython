def factorial(x: i32) -> i32:
    """
    Computes the factorial of `x`.
    """

    if x < 0:
        return 0
    result: i32
    result = 1
    while x > 0:
        result = result * x
        x -= 1
    return result
