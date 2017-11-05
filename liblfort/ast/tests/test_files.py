from glob import glob
import os

from liblfort.ast import parse_file

def test_files():
    here = os.path.dirname(__file__)
    filenames = glob(os.path.join(here, "..", "..", "..", "examples", "*.f90"))
    assert len(filenames) == 9
    print("Testing filenames:")
    skip = ["random.f90"]
    for filename in filenames:
        if os.path.basename(filename) in skip: continue
        print(filename)
        parse_file(filename)
