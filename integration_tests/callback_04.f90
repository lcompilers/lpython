module dr_fortran_cb

contains

  subroutine print_dr()
    print *,'Dr. '
  end subroutine

  subroutine print_fortran() 
    print *,'Fortran'
  end subroutine

  subroutine print_dr_fortran(title_or_name)
    interface
      subroutine title_or_name()
      end subroutine
    end interface
    call title_or_name()
  end subroutine

end module

program main
  use dr_fortran_cb
  call print_dr_fortran(print_dr)
  call print_dr_fortran(print_fortran)
end program
