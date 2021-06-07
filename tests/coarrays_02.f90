program coarrays_02
! Source https://j3-fortran.org/doc/year/18/18-007r1.pdf Page no - 541
    use, intrinsic :: iso_fortran_env
    integer, allocatable :: node (:) ! tree nodes that this image handles.
    integer, allocatable :: nc (:)
    ! node(i) has nc(i) children.
    integer, allocatable :: parent (:), sub (:)
    ! the parent of node (i) is node (sub (i)) [parent (i)].
    type (event_type), allocatable :: done (:) [:]
    integer :: i, j, status
    ! set up the tree, including allocation of all arrays.
    do i = 1, size (node)
    ! wait for children to complete
        if (nc (i) > 0) then
            event wait (done (i), until_count=nc (i), stat=status)
        if (status/=0) exit
        end if

        ! process node, using data from children.
        if (parent (i)>0) then
            ! node is not the root.
            ! place result on image parent (i) for node node (sub) [parent (i)]
            ! tell parent (i) that this has been done.
            event post (done (sub (i)) [parent (i)], stat=status)
        if (status/=0) exit
        end if
    end do
end program
