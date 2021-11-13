module stdlib_bitsets_64
    implicit none

contains

    logical function all_set(x, num_bits) result(res)
        integer, intent(in) :: x
        integer, intent(in) :: num_bits

        logical, intrinsic :: btest
        integer :: pos

        res = .true.
        do pos = 0, num_bits - 1
            if ( .not. btest(x, pos) ) then
                res = .false.
            end if
        end do
    end function all_set
end module

program intrinsics_26
use stdlib_bitsets_64, only : all_set
implicit none

    integer :: i = 31
    print *, all_set(i, 5)

end program