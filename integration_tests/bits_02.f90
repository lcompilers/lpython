program bits_02
use iso_fortran_env, only: block_kind => int64, bits_kind  => int32
implicit none

    integer(bits_kind), parameter :: block_size  = bit_size(0_block_kind)

    print *, block_size

end program
