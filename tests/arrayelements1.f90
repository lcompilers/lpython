module my_subs
implicit none
interface my_sum
  module procedure sum_real, sum_int
end interface my_sum
contains
    subroutine sum_real (array, tot)
       real, dimension(:), intent (in) :: array
       real, intent (out) :: tot
       integer :: i
       tot = 1.0
       do i=1, size (array)
          tot = tot * array (i)
       end do
    end subroutine sum_real

    subroutine sum_int (array, tot)
       integer, dimension(:), intent (in) :: array
       integer, intent (out) :: tot
       integer :: i
       tot = 0
       do i=1, size (array)
          tot = tot + array (i)
       end do
    end subroutine sum_int

end module my_subs

program test_dt
use my_subs
implicit none
    type my_type
       integer weight
       real length
    end type my_type
    type (my_type), dimension (:), allocatable :: people
    type (my_type) :: answer
    allocate (people (2))

    people (1) % weight = 1
    people (1) % length = 1.0
    people (2) % weight = 2
    people (2) % length = 2.0

    call my_sum ( people (:) % weight, answer % weight )
    write (*, *)  answer % weight

    call my_sum ( people (:) % length, answer % length )
    write (*, *)  answer % length
end program test_dt
