from glob import glob
import os

from liblfort.ast import parse_file

def test_files():
    here = os.path.dirname(__file__)
    filenames = glob(os.path.join(here, "..", "..", "..", "examples", "*.f90"))
    assert len(filenames) == 9
    print("Testing filenames:")
    for filename in filenames:
        print(filename)
        parse_file(filename)
