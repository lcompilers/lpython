from liblfort.codegen.evaluator import FortranEvaluator

def test_do_loops_fn():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f()
integer :: i, j
j = 0
do i = 1, 10
    j = j + i
end do
f = j
end function
""")
    assert e.evaluate("f()") == 55
    e.evaluate("""\
integer function f2(n)
integer, intent(in) :: n
integer :: i, j
j = 0
do i = 1, n
    j = j + i
end do
f2 = j
end function
""")
    assert e.evaluate("f2(10)") == 55
    assert e.evaluate("f2(20)") == 210

def test_do_loops_interactive():
    e = FortranEvaluator()
    e.evaluate("integer :: i, j")

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10
    j = j + i
end do
""")
    assert e.evaluate("j") == 55

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 10, 1, -1
    j = j + i
end do
""")
    assert e.evaluate("j") == 55

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 9, 2
    j = j + i
end do
""")
    assert e.evaluate("j") == 25

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 9, 1, -2
    j = j + i
end do
""")
    assert e.evaluate("j") == 25

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10, 2
    j = j + i
end do
""")
    assert e.evaluate("j") == 25

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10, 3
    j = j + i
end do
""")
    assert e.evaluate("j") == 22

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 10, 1, -3
    j = j + i
end do
""")
    assert e.evaluate("j") == 22

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 1
    j = j + i
end do
""")
    assert e.evaluate("j") == 1

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 1, -1
    j = j + i
end do
""")
    assert e.evaluate("j") == 1

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 0
    j = j + i
end do
""")
    assert e.evaluate("j") == 0

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 0, 1, -1
    j = j + i
end do
""")
    assert e.evaluate("j") == 0

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10
    j = j + i
    if (i == 2) exit
end do
""")
    assert e.evaluate("j") == 3

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10
    if (i == 2) exit
    j = j + i
end do
""")
    assert e.evaluate("j") == 1

    e.evaluate("j = 0")
    e.evaluate("""\
do i = 1, 10
    if (i == 2) cycle
    j = j + i
end do
""")
    assert e.evaluate("j") == 53