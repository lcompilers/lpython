def get_string() -> str:
    return "1"

def test() -> list[str]:
    x: list[str]
    x.append(get_string())
    return x

assert test() == ["1"]
