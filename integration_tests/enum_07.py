from enum_07_module import Constants

def check():
    assert Constants.NUM_ELEMS.value == 4
    assert Constants.NUM_CHECK.value == 51

check()
