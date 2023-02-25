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

def title():
    s: str
    s = "tom and jerry"
    assert s.title() == "Tom And Jerry"
    s = "12wddd"
    assert s.title() == "12Wddd"
    s = " tom and jerry"
    assert s.title() == " Tom And Jerry"
    s = "empty string"
    assert s.title() == "Empty String"
    s = "hello b2b2b2 and 3g3g3g"
    assert s.title() == "Hello B2B2B2 And 3G3G3G"
    s = "hello-world"
    assert s.title() == "Hello-World"
    s = ""
    assert s.title() == ""
    s = "rAndOM CApS"
    assert s.title() == "Random Caps"
    assert "empty string" .title() == "Empty String"
    assert "".title() == ""
    assert "RANdom CApS" .title() == "Random Caps"

def lower():
    s: str
    s = "AaaaAABBbbbbBB!@12223BN"
    assert s.lower() == "aaaaaabbbbbbbb!@12223bn"
    assert "DDd12Vv" .lower() == "ddd12vv"
    assert "".lower() == ""

def upper():
    s: str
    s = "AaaaAABBbbbbBB!@12223BN"
    assert s.upper() == "AAAAAABBBBBBBB!@12223BN"
    assert "DDd12Vv" .upper() == "DDD12VV"
    assert "".upper() == ""

def strip():
    s: str
    s = "     AASAsaSas    "
    assert s.rstrip() == "     AASAsaSas"
    assert s.lstrip() == "AASAsaSas    "
    assert s.strip() == "AASAsaSas"
    assert "     AASAsaSas    " .rstrip() == "     AASAsaSas"
    assert "     AASAsaSas    " .lstrip() == "AASAsaSas    "
    assert "     AASAsaSas    " .strip() == "AASAsaSas"
    assert "".strip() == ""


def swapcase():
    s: str
    s = "aaaaaabbbbbbbb!@12223bn"
    assert s.swapcase() == "AAAAAABBBBBBBB!@12223BN"
    assert "AASAsaSas" .swapcase() == "aasaSAsAS"
    assert "".swapcase() == ""


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
    assert "".find("") == 0


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
    assert "".startswith("ok") == False


def check():
    capitalize()
    title()
    lower()
    upper()
    strip()
    swapcase()
    find()
    startswith()

check()
