from lfortran.ast.utils import make_tree

def test_tree1():
    assert make_tree("a", []) == """\
a\
"""
    assert make_tree("a", ["1"]) == """\
a
╰─1\
"""
    assert make_tree("a", ["1", "2"]) == """\
a
├─1
╰─2\
"""
    t = make_tree("a", ["1", "2", "3"])
    assert t  == """\
a
├─1
├─2
╰─3\
"""
    assert make_tree("a", [t, "2", "3"]) == """\
a
├─a
│ ├─1
│ ├─2
│ ╰─3
├─2
╰─3\
"""
    assert make_tree("a", ["1", t, "3"]) == """\
a
├─1
├─a
│ ├─1
│ ├─2
│ ╰─3
╰─3\
"""
    assert make_tree("a", ["1", "2", t]) == """\
a
├─1
├─2
╰─a
  ├─1
  ├─2
  ╰─3\
"""
    assert make_tree("a", ["1", t]) == """\
a
├─1
╰─a
  ├─1
  ├─2
  ╰─3\
"""
    assert make_tree("a", [t]) == """\
a
╰─a
  ├─1
  ├─2
  ╰─3\
"""
    assert make_tree("a", ["1\n2", "3"]) == """\
a
├─1
│ 2
╰─3\
"""
    assert make_tree("a", ["1\n\n\n\n2", "3"]) == """\
a
├─1
│ 
│ 
│ 
│ 2
╰─3\
"""