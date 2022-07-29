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
