program where_01
implicit none
real :: a(10), b(10)
where (a >= 0)
    b = 1
else where
    b = 0
end where

where (a >= 0)
    b = 1
elsewhere
    b = 0
end where

where (a >= 0)
    b = 1
elsewhere
    b = 0
endwhere
end program
