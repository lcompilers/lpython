def capitalize():
    s: str
    s = "tom and jerry"
    assert s.capitalize() == "Tom and jerry"
    s = "12wddd"
    assert s.capitalize() == s
    s = " tom and jerry"
    assert s.capitalize() == s
    assert "empty string" .capitalize() == "Empty string"
    assert "".capitalize() == ""

def lower():
    s: str
    s = "AaaaAABBbbbbBB!@12223BN"
    assert s.lower() == "aaaaaabbbbbbbb!@12223bn"
    assert "DDd12Vv" .lower() == "ddd12vv"

capitalize()
lower()
