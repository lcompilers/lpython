from lpython import i32, f64, pythoncall, Const
from numpy import empty, int32, float64


@pythoncall(module = "bindpy_06_module")
def get_cpython_version() -> str:
    pass


@pythoncall(module = "bindpy_06_module")
def get_modified_dict(d: dict[str, i32]) -> dict[str, i32]:
    pass


@pythoncall(module = "bindpy_06_module")
def get_modified_list(d: list[str]) -> list[str]:
    pass

@pythoncall(module = "bindpy_06_module")
def get_modified_tuple(t: tuple[i32, i32]) -> tuple[i32, i32, i32]:
    pass


@pythoncall(module = "bindpy_06_module")
def get_modified_set(s: set[i32]) -> set[i32]:
    pass


def test_list():
    l: list[str] = ["LPython"]
    lr: list[str] = get_modified_list(l)
    assert len(lr) == 2
    assert lr[0] == "LPython"
    assert lr[1] == "LFortran"


def test_tuple():
    t: tuple[i32, i32] = (2, 4)
    tr: tuple[i32, i32, i32] = get_modified_tuple(t)
    assert tr[0] == t[0]
    assert tr[1] == t[1]
    assert tr[2] == t[0] + t[1]


def test_set():
    s: set[i32] = {1, 2, 3}
    sr: set[i32] = get_modified_set(s)
    assert len(sr) == 4
    assert 1 in sr
    assert 2 in sr
    assert 3 in sr
    assert 100 in sr


def test_dict():
    d: dict[str, i32] = {
        "LPython": 50
    }
    dr: dict[str, i32] = get_modified_dict(d)
    assert len(dr) == 2
    assert dr["LPython"] == 50
    assert dr["LFortran"] == 100


def main0():
    test_list()
    test_tuple()
    test_set()
    test_dict()


main0()
