program cond_01
implicit none
if (.false.) error stop
if (1 == 2) error stop
if (1 /= 1) error stop
if (1 > 2) error stop
if (1 >= 2) error stop
if (2 < 1) error stop
if (2 <= 1) error stop
end
