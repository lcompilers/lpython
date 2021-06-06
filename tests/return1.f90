subroutine example (s)
    character s* 32
    write (*,*) s
    return
end
! Syntax check only
subroutine example1 (i, a, b, c)
    if (i .eq. a) return 1
    if (i .eq. b) return 2
    if (i .eq. c) return 3
end
