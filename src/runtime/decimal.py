from ltypes import i8, i16, i32, f32, f64, ccall

@overload
def Decimal(x: i64) -> i64:
    return x

@overload
def Decimal(x: i32) -> i32:
    return x

@overload
def Decimal(x: f32) -> f64:
    return float(x)
@overload
def Decimal(x: f64) -> f64:
    return float(x)

# The following to be implemented once the above code works.

# @overload
# def Decimal(x: str) -> f64:
#     return float(x)

# @overload
# def Decimal(x: str) -> i64:
#     return int(x)