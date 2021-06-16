block data init
    implicit integer(a-z)
    common /range/ x0, x1
    data x0, x1 / 1, 10 /
end block data

program block_data1
    implicit integer(a-z)
    common /range/ x0, x1
    print *, "Printing Even number in the Range: ", x0, " to ", x1
    do i = x0, x1
        if(mod(i, 2) /= 0) cycle
        write(*,*) i
    end do
end program
