function conform(mold, val, dim)
   type(*), intent(in) :: mold(..)
   type(*), intent(in), optional :: val(..)
   integer, intent(in), optional :: dim
   logical :: conform
   if (present(val)) then
      if (present(dim)) then
         if (dim > 0 .and. dim <= rank(mold) .and. dim <= rank(val)) then
            conform = size(mold, dim=dim) == size(val, dim=dim)
         else
            error stop "Runtime error: Illegal dim argument provided in conform"
         end if
      else
         if (rank(val) == rank(mold)) then
            conform = all(shape(mold) == shape(val))
         else
            conform = .false.
         end if
      end if
   else
      conform = .true.
   end if
end function
