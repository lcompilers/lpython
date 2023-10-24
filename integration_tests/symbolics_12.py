from sympy import Symbol, E, log, exp
from lpython import S

def main0():
    # Define symbolic variables
    x: S = Symbol('x')
    y: S = Symbol('y')

    # Assign E to the variable x
    x = E

    # Check if x is equal to E
    assert x == E

    # Perform some symbolic operations
    z: S = x + y

    # Check if z is equal to E + y
    assert z == E + y

    # Check if x is not equal to 2E + y
    assert x != S(2) * E + y

    # Evaluate some mathematical expressions
    expr1: S = log(E)
    expr2: S = exp(S(1))

    # Check the results
    assert expr1 == S(1)
    assert expr2 == E ** S(1)

    # Print the results
    print("x =", x)
    print("z =", z)
    print("log(E) =", expr1)
    print("exp(1) =", expr2)


main0()
