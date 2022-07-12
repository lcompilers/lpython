from test_vars_01b import test_name_other_module

def test_name():
    print(__name__)
    assert __name__ == "__main__"

test_name()
test_name_other_module()
