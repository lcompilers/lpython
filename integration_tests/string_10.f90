program string_10
    character(len=2) :: c = "BC"
    logical :: is_alpha
    is_alpha = (c >= 'A' .and. c <= 'Z') .or. (c >= 'a' .and. c <= 'z')
    print *, is_alpha
    c = "@a"
    is_alpha = (c >= 'A' .and. c <= 'Z') .or. (c >= 'a' .and. c <= 'z')
    print *, is_alpha
    c = "a@"
    is_alpha = (c >= 'A' .and. c <= 'Z') .or. (c >= 'a' .and. c <= 'z')
    print *, is_alpha
end program string_10
