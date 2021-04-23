module modules_08_b
implicit none
private
public b

contains

integer function b()
b = 5 + kind(.true.)
end function

end module
