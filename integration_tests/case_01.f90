program case_01
implicit none
integer :: i
i = 4

select case(i)
    case (1)
        print *, "1"
    case (2)
        print *, "2"
    case (3)
        print *, "3"
    case (4)
        print *, "4"
end select

selectcase(i)
    case (1)
        print *, "1"
    case (2,3,4)
        print *, "2,3,4"
end select

end
