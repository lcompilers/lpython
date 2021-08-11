module hr
    implicit none
    type, public :: person
        character(len=20) :: first, last
        integer :: birthyear
        character(len=1) :: sgender
    end type person

    type, public :: employee
        type(person) :: person
        integer :: hire_date
        character(len=20) :: department
    end type employee
end module hr


program hr_code
    use hr, only: person, employee
    implicit none

    type(person) :: jack
    type(employee) :: jill
    jack = person( "Jack", "Smith", 1984, "M" )
    jill = employee( person( "Jill", "Smith", 1984, "F" ), 2003, "sales" )
    
    print *, jack%first, jack%last, jack%birthyear, jack%sgender
    print *, jill%person%first, jill%person%last, jill%person%birthyear, jill%person%sgender, jill%department, jill%hire_date
end program hr_code
