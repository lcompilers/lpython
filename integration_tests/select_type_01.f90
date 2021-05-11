program select_type_01
implicit none
  type base
    integer::i
  end type

  type, extends(base)::child
    integer::j
  end type

  class(base), pointer :: bptr
  type(base), target :: base_target = base(10)
  type(child), target :: child_target = child(20, 30)

  bptr => child_target

  select type(bptr)
    type is (base)
    print *, "base type: component value: ", bptr%i
    type is (child)
    print *, "child type: component values: ", bptr%i, bptr%j
  end select

  bptr => base_target

  select type(bptr)
    type is (base)
    print *, "base type: component value: ", bptr%i
    type is (child)
    print *, "child type: component values: ", bptr%i, bptr%j
  end select

end program select_type_01
