def test_name_other_module():
    print(__name__)
    assert __name__ != "__main__"
