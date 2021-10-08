module Complex_module
    implicit none
    type :: ComplexType
        real :: r
        real :: i

        contains

        procedure :: integer_add_subrout
        procedure :: real_add_subrout
        generic :: add => integer_add_subrout, real_add_subrout
    end type

    contains

    subroutine integer_add_subrout(this, r, i, sum)
        class(ComplexType), intent(in) :: this
        integer, intent(in) :: r, i
        type(ComplexType), intent(out) :: sum
        print *, "Calling integer_add_subrout"
        sum%r = this%r + r
        sum%i = this%i + i
    end subroutine

    subroutine real_add_subrout(this, r, i, sum)
        class(ComplexType), intent(in) :: this
        real, intent(in) :: r, i
        type(ComplexType), intent(out) :: sum
        print *, "Calling real_add_subrout"
        sum%r = this%r + r
        sum%i = this%i + i
    end subroutine

end module

program generic_name_01
    use Complex_module, only: ComplexType
    implicit none
  
    type(ComplexType) :: a, c
    c = ComplexType(1.0, 2.0)
    call c%add(1, 0, a)
    print *, a%r, a%i
    call c%add(0.0, -1.0, a)
    print *, a%r, a%i
end program
