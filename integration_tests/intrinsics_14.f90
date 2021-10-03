program intrinsics_14
integer, parameter :: &
    k1 = selected_int_kind(5), &
    k2 = selected_int_kind(5_4), &
    k3 = selected_int_kind(5_8), &
    k4 = selected_int_kind(8), &
    k5 = selected_int_kind(10), &
    k6 = selected_real_kind(5), &
    k7 = selected_real_kind(5_4), &
    k8 = selected_real_kind(5_8), &
    k9 = selected_real_kind(8), &
    k10 = selected_char_kind("default")

print*,selected_int_kind(5), k1 ! 4
print*,selected_int_kind(5_4), k2 ! 4
print*,selected_int_kind(5_8), k3 ! 4
print*,selected_int_kind(8), k4 ! 4
print*,selected_int_kind(10), k5 ! 8
print*,selected_real_kind(5), k6 ! 4
print*,selected_real_kind(5_4), k7 ! 4
print*,selected_real_kind(5_8), k8 ! 4
print*,selected_real_kind(8), k9 ! 8
print*,selected_char_kind("default"), k10 ! 1
end program
