subroutine process(x)
    real x(..)
    select rank(x)
        rank (0)
            x = 0
        rank (2)
            if (size(x,2)>=2) x(:,2) = 2
        rank default
            print *, 'i did not expect rank', rank(x), 'shape', shape(x)
            error stop 'process bad arg'
    end select
    return
end subroutine process

subroutine initialize (arg, size)
    real,contiguous :: arg(..)
    integer         :: size, i
    select rank (arg)
        rank (0)   ! special case the scalar case
            arg = 0.0
        rank (*)
            do i = 1, size
                arg(i) = 0.0
            end do
    end select
    return
end subroutine
