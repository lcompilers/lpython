from ltypes import i8, i16, i32, f32, f64, ccall


@dataclass
class Decimal:
    value: str

    @overload
    def __init__(self, value_: i64) -> i64:
        return self.value

    @overload
    def __init__(self, value_: i32) -> i64:
        return self.value

    @overload
    def __init__(self, value_: f32) -> f64:
        result: f64
        result = self.value
        return result

    @overload
    def __init__(self, value_: f64) -> f64:
        return float(self.value)


# @overload
# def Decimal(x: i64) -> i64:
#     return x

# @overload
# def Decimal(x: i32) -> i32:
#     return x

# @overload
# def Decimal(x: f32) -> f64:
#     result:f64
#     result = x
#     return result
# @overload
# def Decimal(x: f64) -> f64:
#     return float(x)

# The following to be implemented once the above code works.

# @overload
# def Decimal(x: str) -> f64:
#     return float(x)

# @overload
# def Decimal(x: str) -> i64:
#     return int(x)