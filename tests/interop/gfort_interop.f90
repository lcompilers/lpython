module gfort_interop
use iso_c_binding, only: c_int, c_loc, c_ptr, c_ptrdiff_t, c_size_t, c_int32_t
implicit none

type, bind(c) :: descriptor_dimension
    integer(c_ptrdiff_t) :: stride
    integer(c_ptrdiff_t) :: lower_bound
    integer(c_ptrdiff_t) :: upper_bound
end type

type, bind(c) :: c_desc1_t
    type(c_ptr) :: base_addr
    integer(c_size_t) :: offset
    integer(c_ptrdiff_t) :: dtype
    type(descriptor_dimension) :: dim(1)
end type

type, bind(c) :: c_desc2_t
    type(c_ptr) :: base_addr
    integer(c_size_t) :: offset
    integer(c_ptrdiff_t) :: dtype
    type(descriptor_dimension) :: dim(2)
end type

interface c_desc
    module procedure c_desc1_int32
    module procedure c_desc2_int32
end interface

integer, parameter :: INTEGER_ELEMENT_TYPE = 1
integer, parameter :: LOGICAL_ELEMENT_TYPE = 2
integer, parameter :: REAL_ELEMENT_TYPE = 3
integer, parameter :: COMPLEX_ELEMENT_TYPE = 4
integer, parameter :: DERIVED_TYPE_ELEMENT_TYPE = 5
integer, parameter :: CHARACTER_ELEMENT_TYPE = 6

integer, parameter :: c_int8_t_size = 1
integer, parameter :: c_int16_t_size = 2
integer, parameter :: c_int32_t_size = 4
integer, parameter :: c_int64_t_size = 8
integer, parameter :: c_int128_t_size = 16

contains

type(c_desc1_t) function c_desc1_int32(A) result(desc)
integer(c_int32_t), intent(in), target :: A(:)
desc%base_addr = c_loc(A(1))
desc%dim(1)%stride = 1 ! A must be contiguous
desc%dim(1)%lower_bound = lbound(A,1)
desc%dim(1)%upper_bound = ubound(A,1)
desc%dtype = size(shape(A)) ! Dimension
desc%dtype = ior(desc%dtype, lshift(INTEGER_ELEMENT_TYPE, 3))
desc%dtype = ior(desc%dtype, lshift(c_int32_t_size, 6))
desc%offset = -(desc%dim(1)%lower_bound * desc%dim(1)%stride)
end function

type(c_desc2_t) function c_desc2_int32(A) result(desc)
integer(c_int32_t), intent(in), target :: A(:,:)
integer :: n, dim
dim = size(shape(A))
desc%base_addr = c_loc(A(1,1))
desc%offset = 0
do n = 1, dim
    if (n == 1) then
        desc%dim(n)%stride = 1
    else
        desc%dim(n)%stride = &
            (desc%dim(n-1)%upper_bound - desc%dim(n-1)%lower_bound + 1) &
            * desc%dim(n-1)%stride
    end if
    desc%dim(n)%lower_bound = lbound(A,n)
    desc%dim(n)%upper_bound = ubound(A,n)
    desc%offset = desc%offset &
        - desc%dim(n)%lower_bound * desc%dim(n)%stride
end do
desc%dtype = dim
desc%dtype = ior(desc%dtype, lshift(INTEGER_ELEMENT_TYPE, 3))
desc%dtype = ior(desc%dtype, lshift(c_int32_t_size, 6))
end function

end module
