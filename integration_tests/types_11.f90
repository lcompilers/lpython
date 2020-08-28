program types_11
implicit none

enum, bind(c)
    enumerator :: a = 1
    enumerator :: b = 2
    enumerator :: c = 3
end enum

end program
