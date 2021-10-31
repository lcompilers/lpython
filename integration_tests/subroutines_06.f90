module bitset
    interface assignment(=)
        pure module subroutine assign_large( set1, set2 )
            integer, intent(out) :: set1
            integer, intent(in)  :: set2
        end subroutine assign_large

        pure module subroutine assign_logint8_large( self, logical_vector )
            integer, intent(out) :: self
            logical, intent(in)       :: logical_vector(:)
        end subroutine assign_logint8_large
    end interface
end module

program subroutines_06

    use bitset
    implicit none

    ! empty program

end program subroutines_06