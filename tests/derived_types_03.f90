program derived_types_03
! AST only
DO nn = 1, ndiag
    ing = diag(nn)%len
    ALLOCATE( diag(nn)%cell_id(ing), STAT=ierr )
    IF ( ierr /= 0 ) RETURN
    ndpwds = ndpwds + SIZE( diag(nn)%cell_id )
END DO

DO k = 1, nz
DO j = 1, ny
DO i = 1, ichunk
    nn = i + j + k - 2
    indx(nn) = indx(nn) + 1
    ing = indx(nn)
    diag(nn)%cell_id(ing)%ic = i
    diag(nn)%cell_id(ing)%j  = j
    diag(nn)%cell_id(ing)%k  = k
END DO
END DO
END DO
end program
