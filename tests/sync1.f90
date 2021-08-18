program sync1
implicit none
integer :: iam
real    :: x[*]
iam = this_image()

if (iam == 1) x = 1.0
sync memory

call external_sync ()
syncmemory(stat=status)

if (iam == 2) write (*,*) x[1]

if (this_image() == 1) then
    sync images(*)
else
    syncimages(1, stat=status)
end if

end program sync1
