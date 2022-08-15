def capitalize():
    s: str
    s = "tom and jerry"
    assert s.capitalize() == "Tom and jerry"
    s = "12wddd"
    assert s.capitalize() == s
    s = " tom and jerry"
    assert s.capitalize() == s

capitalize()

