def main0():
    x: str
    x = "Hello, World"

    assert "Hello" in x
    assert "," in x
    assert "rld" in x

    assert "Hello" not in "World"

main0()
