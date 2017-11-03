from glob import glob
from liblfort.ast import parse_file

def test_files():
    filenames = glob("examples/*.f90")
    assert len(filenames) == 9
    print("Testing filenames:")
    for filename in filenames:
        print(filename)
        assert parse_file(filename)
