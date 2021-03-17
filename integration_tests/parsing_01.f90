module submodule
   implicit none
   integer :: complex = 1
   interface
      module subroutine function(integer,real)
         integer, intent(in) :: integer
         integer, intent(out) :: real
      end subroutine function
   end interface
   contains
end module submodule

submodule (submodule) module
   contains
      module procedure function
         real = 2*integer
      end procedure function
end submodule module

program program

   use submodule, only: character => complex, subroutine => function
   implicit none

   integer :: integer, real
   integer = character
   associate: associate (logical=>character,complex=>real)
      call subroutine(logical,complex)
      print*, (complex)
   end associate associate

end program program
