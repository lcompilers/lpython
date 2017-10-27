from glob import glob
from fortran_parser import parse_file

def test_files():
    filenames = glob("examples/*.f90")
    assert len(filenames) == 8
    print("Testing filenames:")
    for filename in filenames:
        print(filename)
        assert parse_file(filename)
