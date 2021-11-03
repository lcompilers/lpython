program bits_02
use iso_fortran_env, only: block_kind => int64, bits_kind  => int32
implicit none

    integer(bits_kind), parameter :: block_size  = bit_size(0_block_kind)
    integer(block_kind), parameter :: all_zeros  = 0_block_kind
    integer(block_kind), parameter :: all_ones   = not(all_zeros)

    print *, block_size
    print *, all_zeros
    print *, all_ones

end program
