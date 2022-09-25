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
