program main
    implicit none

    ! Base types -----------
    
    type :: bar_a
      integer :: a
    end type bar_a
    
    type :: foo_a
      type(bar_a) :: m_bar_a
    end type foo_a

    ! Extended types -------

    type, extends(bar_a) :: bar_b
      integer :: b
    end type bar_b

    type, extends(foo_a) :: foo_b
      type(bar_b) :: m_bar_b ! <-- Component ‘bar’ at (1) already in the parent type
    end type foo_b
    
    ! ----------------------
  
    type(foo_b) :: foo


    foo%m_bar_a%a = 1
    foo%m_bar_b%b = 2

    print *, foo%m_bar_a%a
    print *, foo%m_bar_b%b
  
  end program main