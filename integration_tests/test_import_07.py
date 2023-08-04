# test issue 2153
from test_import_07_module import f as fa

def main0():
    assert fa(3) == 6
    assert fa(10) == 20

main0()
