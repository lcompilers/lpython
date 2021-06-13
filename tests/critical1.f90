program critical1
implicit none
integer :: joblist[*], job
integer :: a, b
if (this_image() == 1) read(*,*) joblist
sync all
do
    critical
      job = joblist[1]
      joblist[1] = job - 1
    end critical

    if (job > 0) then
        b = a**2 + b**2
    else
       exit
    end if
end do
sync all
end program
