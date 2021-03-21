module submodule
   implicit none
   integer :: complex = 1
   logical :: true = .TRUE.
   logical :: false = .FALSE.

   type :: type
      type(type), allocatable :: types(:)
   end type 

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

   use submodule, only: character => complex, subroutine => function, &
      then => true, else => false, type
   implicit none

   integer :: integer, real
   type(type) :: types

   integer = character

   ! Thanks to https://stackoverflow.com/a/57015100/1876449
   types = type(types=[type(types=[type(), type(), type()]), type()])

   block: block
      logical :: if
      endif: if (then) then
         if = then
      else if (else) then
         if = else
      else
         if = .not.(else) .and. .not.(if)
      endif endif
      print *, if
   end block block

   associate: associate (logical=>character,complex=>real)
      call subroutine(logical,complex)
      print*, (complex)
   end associate associate

end program program
