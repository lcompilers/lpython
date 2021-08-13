program array9
    type :: varying_string
       character(len=1), allocatable :: characters(:)
    endtype
    
    type(varying_string) :: x
    type(varying_string), allocatable :: a(:)
    real, allocatable :: b(:)
    integer, allocatable :: c(:)
    real(kind=4), allocatable :: d(:)
    
    a = [varying_string::]
    b = [real::]
    b = [integer::]
    d = [real(kind=4)::]

    a = [varying_string::x,x]
    b = [real::1.0,2.0]
    b = [integer::1,2]
    d = [real(kind=4)::1.0,2.0]
endprogram
    
