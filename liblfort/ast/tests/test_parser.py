from liblfort.ast import parse

def test_parse():
    node = parse("1+1")
    print(node)
