program forall1
implicit none
! Only for Syntax check

!>>>>>>>>>>>>> Single line statments <<<<<<<<<<<<<<
    FORALL (i=1:n) A(i,i) = B(i)
    forall (i=1:n:3, j=1:n:5) A(i,j) = SIN(A(j,i))
    forall (i=1:n, j=1:n, i/=j) A(i,j) = REAL(i+j)
    forall (i=1:n, j=1:m, A(i,j).NE.0) &
           A(i,j) = 1/A(i,j)
    forall (i=1:1000, j=1:1000, i /= j) A(i,j) = A(j,i)

!>>>>>>>>>>>>> Multipe line statements <<<<<<<<<<<<<<
    forall (j = 1:n) shared(i) local(x) default(none)
        forall (i=1:j) A(i,j) = B(i)
    end forall

    forall (i = 1:N) reduce(*: s)
        s = s + a(i)
    end forall

    FORALL(i = 3:N + 1, j = 3:N + 1)
        C(i, j) = C(i, j + 2) + C(i, j - 2)
        D(i, j) = C(i, j) + C(i + 2, j) + C(i - 2, j)
    end FORALL

    forall (x=1:100, J(x)>0)
        where (I(x,:)<0)
            I(x,:)=0
        elsewhere
            I(x,:)=1
        end where
    end forall

    outer: forall (i=1:100)
        inner: forall (j=1:100, i.NE.j)
            A(i,j) = A(j,i)
        end forall inner
    end forall outer
end program
