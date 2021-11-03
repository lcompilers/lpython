module bitset
    type, abstract :: bitset_type

        private
        integer(8) :: num_bits

    contains

        procedure(all_abstract), deferred, pass(self) :: all

    end type bitset_type


    abstract interface

        elemental function all_abstract( self ) result(all)
            import :: bitset_type
            logical :: all
            class(bitset_type), intent(in) :: self
        end function all_abstract
    end interface

end module

program debug
    implicit none
end program