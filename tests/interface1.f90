module interface1
implicit none

interface randn
    module procedure randn_scalar
    module procedure :: randn_vector
    module procedure randn_matrix
    module procedure :: randn_vector_n
end interface

end module
