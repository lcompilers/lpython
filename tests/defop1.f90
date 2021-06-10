program defop1
! Tests for syntax (AST) only:
e = a.in.b
e = b%s.op.3**4
e = (a+b).x.y**x
e = x**x.x.x**x
e = ((x.x.x).in.3)**4
end program
