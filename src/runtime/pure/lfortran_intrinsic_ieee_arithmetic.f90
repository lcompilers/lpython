module lfortran_intrinsic_ieee_arithmetic
    use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
    implicit none

    type ieee_class_type
    end type

    type(ieee_class_type) :: ieee_negative_denormal
    type(ieee_class_type) :: ieee_negative_inf
    type(ieee_class_type) :: ieee_negative_normal
    type(ieee_class_type) :: ieee_negative_zero
    type(ieee_class_type) :: ieee_positive_denormal
    type(ieee_class_type) :: ieee_positive_inf
    type(ieee_class_type) :: ieee_positive_normal
    type(ieee_class_type) :: ieee_positive_zero
    type(ieee_class_type) :: ieee_quiet_nan
    type(ieee_class_type) :: ieee_signaling_nan

    interface ieee_class
        module procedure spieee_class, dpieee_class
    end interface

    interface ieee_value
        module procedure spieee_value, dpieee_value
    end interface

    interface ieee_is_nan
        module procedure spieee_is_nan, dpieee_is_nan
    end interface

    contains

    elemental function spieee_class(x) result(y)
        real(sp), intent(in) :: x
        type(ieee_class_type) :: y
    end function

    elemental function dpieee_class(x) result(y)
        real(dp), intent(in) :: x
        type(ieee_class_type) :: y
    end function

    elemental function spieee_value(x, cls) result(y)
        real(sp), intent(in) :: x
        type(ieee_class_type), intent(in) :: cls
        real(sp) :: y
    end function

    elemental function dpieee_value(x, cls) result(y)
        real(dp), intent(in) :: x
        type(ieee_class_type), intent(in) :: cls
        real(dp) :: y
    end function

    elemental function spieee_is_nan(x) result(y)
        real(sp), intent(in) :: x
        logical :: y
    end function

    elemental function dpieee_is_nan(x) result(y)
        real(dp), intent(in) :: x
        logical :: y
    end function

end module