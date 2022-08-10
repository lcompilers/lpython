def main0(i, ):
    pass


def example(
    text,
    width=80,
    fill_char='-',
):                  # type: (...) -> str
    return


example(text, width=80, fill_char='-',)

def test_1(x):  # type: ignore[misc]
    pass

def test_2(y):  # type: ignore
    pass


def func(
    a,          # type: str
    b=80,       # type: int
    c: str = '-',      # type: str
):
    pass


def abc(

    a,
    b,       # type: int
    c=['-'],  # type: list[str]


):                  # type: (...) -> str
    return


def test_01(a,  #   type:int
    b=0
):
    pass


def test_02(a,  #   type:int
    b):
    pass

def quantiles(dist, /, *, n):
    ...

def quantiles(dist, /, *, n=4, method='exclusive'):
    ...

def func(self, param1, param2, /, param3, *, param4, param5):
    ...

def func(self, param1, param2, /, param3=7, *, param4, param5):
    ...

def func(self, param1, param2, /, param3, param3_1=2, *, param4, param5):
    ...


def add(a, b, *, c, d):
    ...

def func(*, param4, param5):
    ...

def func(*, param4, param5): ...;

async def main(): a = b; return a,

def dtype(self) -> _DType_co: ...
