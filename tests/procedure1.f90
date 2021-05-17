submodule (submodule) example
contains
    module procedure distance
        import, all
        implicit none
        distance = sqrt((x1 - x2)**2 + (y1 - y2)**2)
    end procedure distance
end submodule example

submodule (submodule) module
contains
    module procedure real
        import none
        implicit none
        real = 2*integer
    end procedure real
end submodule module

submodule (submodule) module1
contains
    module procedure b
        import :: a, b
        implicit none
        b = (a + b)**2
    end procedure b
end submodule module1

submodule (submodule) module2
contains
    module procedure function
        import, only: a, b
        implicit none
    end procedure function
end submodule module2
