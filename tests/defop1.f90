program defop1
! Tests for syntax (AST) only:
e = a.in.b
E = A.IN.B
e = b%s.op.3**4
E = B%S.OP.3**4
e = (a+b).x.y**x
e = x**x.x.x**x
E = X**X.X.X**X
e = ((x.x.x).in.3)**4
end program
