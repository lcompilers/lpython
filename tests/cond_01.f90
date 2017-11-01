program cond_01
implicit none
if (.false.) stop 1
if (1 == 2) stop 1
if (1 /= 1) stop 1
if (1 > 2) stop 1
if (1 >= 2) stop 1
if (2 < 1) stop 1
if (2 <= 1) stop 1
end
