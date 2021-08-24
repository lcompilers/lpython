program intrinsics_04s
complex :: z

z = (1.5, 0.1)
z = tan(z)
print *, z
if (abs(real(z) - 4.69238520) > 1e-5) error stop
!if (abs(aimag(z) - 6.69462872) > 1e-5) error stop

end
