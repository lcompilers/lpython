program intrinsics_14
  print*,selected_int_kind(5) ! 4
  print*,selected_int_kind(5_4) ! 4
  print*,selected_int_kind(5_8) ! 4
  print*,selected_int_kind(8) ! 4
  print*,selected_int_kind(10) ! 8
  ! TODO: selected_real_kind() can take upto 3 (optional) integer commands
  print*,selected_real_kind(5) ! 4
  print*,selected_real_kind(5_4) ! 4
  print*,selected_real_kind(5_8) ! 4
  print*,selected_real_kind(8) ! 8
end program
