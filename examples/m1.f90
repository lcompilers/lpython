module m1
implicit none
private
public sp, dp, hp

integer, parameter :: dp=kind(0.d0), &             ! double precision
                      hp=selected_real_kind(15), & ! high precision
                      sp=kind(0.)                  ! single precision

end module
