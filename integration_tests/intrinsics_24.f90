program intrinsics_24
implicit none

integer(4) :: count, rate, max
integer(8) :: count_64, rate_64, max_64

call system_clock(count, rate, max)
write(*,*) count                    ! writes current count
write(*,*) rate                     ! writes count rate
write(*,*) max                      ! writes maximum count possible
write(*,*) real(count)/real(rate)   ! current count in seconds

call system_clock(count_64, rate_64, max_64)
write(*,*) count_64                         ! writes current count
write(*,*) rate_64                          ! writes count rate
write(*,*) max_64                           ! writes maximum count possible
write(*,*) real(count_64)/real(rate_64)     ! current count in seconds

end program
