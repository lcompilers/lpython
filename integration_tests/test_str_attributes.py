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

def strip():
    s: str
    s = "     AASAsaSas    "
    assert s.rstrip() == "     AASAsaSas"
    assert s.lstrip() == "AASAsaSas    "
    assert s.strip() == "AASAsaSas"
    assert "     AASAsaSas    " .rstrip() == "     AASAsaSas"
    assert "     AASAsaSas    " .lstrip() == "AASAsaSas    "
    assert "     AASAsaSas    " .strip() == "AASAsaSas"

def startswith():
    s: str
    s = "   empty"
    assert s.startswith("  ") == True
    assert "  @" .startswith(" ") == True
    assert "   emptyAaaaAABBbbbbBB" .startswith(s) == True
    assert "   emptyAaaaAABBbbbbBB" .startswith("AABB") == False
    assert "   emptyAaaaAABBbbbbBB" .startswith("emptyAaxX") == False

    assert "". startswith("sd") == False
    assert "empty" .startswith("") == True
    assert "".startswith("") == True
    assert s.startswith("") == True
    s = ""
    assert s.startswith("") == True
    assert s.startswith("sdd") == False

capitalize()
lower()
strip()
startswith()
