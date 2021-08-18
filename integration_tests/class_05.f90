program main
    implicit none

    type :: bar_a
      integer :: a
    end type bar_a

    type :: foo_a
      type(bar_a) :: m_bar_a
    end type foo_a

    type, extends(bar_a) :: bar_b
      integer :: b
    end type bar_b

    type, extends(foo_a) :: foo_b
      type(bar_b) :: m_bar_b
    end type foo_b

    type(foo_b) :: foo

    foo%m_bar_a%a = -20
    foo%m_bar_b%b = 20

    print *, foo%m_bar_a%a
    print *, foo%m_bar_b%b

    if( foo%m_bar_a%a + foo%m_bar_b%b /= 0 ) error stop

  end program main