program test_fortran
implicit none
integer :: a(3)
a = [1, 2, 3]
call assert(f2b(a) == 6)
call assert(f2b([1, 2, 3, 4, 5, 6]) == 21)
call assert(f3b(reshape([1, 2, 3, 4, 5, 6], [2,3])) == 21)

contains

    integer function f2b(a) result(r)
    use gfort_interop, only: c_desc
    integer, intent(in) :: a(:)
    interface
        integer(c_int) function f2b_c_wrapper(a) bind(c, name="__mod1_MOD_f2b")
        use iso_c_binding, only: c_int
        use gfort_interop, only: c_desc1_t
        type(c_desc1_t), intent(in) :: a
        end function
    end interface
    r = f2b_c_wrapper(c_desc(a))
    end function

    integer function f3b(a) result(r)
    use gfort_interop, only: c_desc
    integer, intent(in) :: a(:,:)
    interface
        integer(c_int) function f3b_c_wrapper(a) bind(c, name="__mod1_MOD_f3b")
        use iso_c_binding, only: c_int
        use gfort_interop, only: c_desc2_t
        type(c_desc2_t), intent(in) :: a
        end function
    end interface
    r = f3b_c_wrapper(c_desc(a))
    end function

    subroutine assert(condition)
    logical, intent(in) :: condition
    if (.not. condition) error stop "Assert failed."
    end subroutine

end program
