module interface2
implicit none

abstract interface
    subroutine read_params(this, params)
    import
    class(porous_drag_model), intent(inout) :: this
    type(parameter_list), pointer, intent(in) :: params
    end subroutine read_params
end interface

INTERFACE ASSIGNMENT(=)
    MODULE PROCEDURE SomeProc
    PROCEDURE SomeProc2
    PROCEDURE :: SomeProc3
END INTERFACE

end module
