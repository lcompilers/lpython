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

def swapcase():
    s: str
    s = "aaaaaabbbbbbbb!@12223bn"
    assert s.swapcase() == "AAAAAABBBBBBBB!@12223BN"
    assert "AASAsaSas" .swapcase() == "aasaSAsAS"

def find():
    s: str
    sub: str
    s = "AaaaAABBbbbbBB!@12223BN"
    sub = "@"
    assert s.find(sub) == 15
    assert s.find('B') == 6
    assert "empty strings" .find("string") == 6
    s2: str
    s2 = "Well copying a string from a website makes us prone to copyright claims. Can you just write something of your own? Like just take this review comment and put it as a string?"
    assert s2.find("of") == 102
    assert s2.find("own") == 110
    assert s2.find("this") == 130
    assert s2.find("") == 0
    assert "".find("dd") == -1
    assert "".find("") == 0
    s2 = ""
    assert s2.find("") == 0
    assert s2.find("we") == -1
    
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
swapcase()
find()
startswith()
