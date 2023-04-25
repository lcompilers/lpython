def is_lower():
    # Case 1: When constant string is present
    assert "".islower() == False
    assert "APPLE".islower() == False
    assert "4432632479".islower() == False
    assert "%#$#$#32a".islower() == True
    assert "apple".islower() == True
    assert "apple is a fruit".islower() == True

    # Case 2: When variable string is present
    s: str
    s = ""
    assert s.islower() == False
    s = "APPLE"
    assert s.islower() == False
    s = "238734587"
    assert s.islower() == False
    s = "%#$#$#32a"
    assert s.islower() == True
    s = "apple"
    assert s.islower() == True
    s = "apple is a fruit"
    assert s.islower() == True

def is_upper():
        # Case 1: When constant string is present
    assert "".isupper() == False
    assert "apple".isupper() == False
    assert "4432632479".isupper() == False
    assert "%#$#$#32A".isupper() == True
    assert "APPLE".isupper() == True
    assert "APPLE IS A FRUIT".isupper() == True

    # Case 2: When variable string is present
    s: str
    s = ""
    assert s.isupper() == False
    s = "apple"
    assert s.isupper() == False
    s = "238734587"
    assert s.isupper() == False
    s = "%#$#$#32A"
    assert s.isupper() == True
    s = "APPLE"
    assert s.isupper() == True
    s = "APPLE IS A FRUIT"
    assert s.isupper() == True

def is_decimal():
    # Case 1: When constant string is present
    assert "".isdecimal() == False
    assert "apple".isdecimal() == False
    assert "4432632479".isdecimal() == True
    assert "%#$#$#32A".isdecimal() == False
    assert "1.25".isdecimal() == False
    assert "-325".isdecimal() == False
    assert "12 35".isdecimal() == False

    # Case 2: When variable string is present
    s: str
    s = ""
    assert s.isdecimal() == False
    s = "apple"
    assert s.isdecimal() == False
    s = "238734587"
    assert s.isdecimal() == True
    s = "%#$#$#32A"
    assert s.isdecimal() == False
    s = "1.35"
    assert s.isdecimal() == False
    s = "-42556"
    assert s.isdecimal() == False
    s = "12 34"
    assert s.isdecimal() == False

def is_ascii():
    # Case 1: When constant string is present
    assert "".isascii() == True
    assert "    ".isascii() == True
    assert "Hello, World123!".isascii() == True
    assert "Hëllö, Wörld!".isascii() == False
    assert "This is a test string with some non-ASCII characters: 🚀".isascii() == False
    assert "\t\n\r".isascii() == True
    assert "12 35".isascii() == True

    # # Case 2: When variable string is present
    s: str
    s = ""
    assert s.isascii() == True
    s = "  "
    assert s.isascii() == True
    s = "Hello, World!"
    assert s.isascii() == True
    s = "Hëllö, Wörld!"
    assert s.isascii() == False
    s = "This is a test string with some non-ASCII characters: 🚀"
    assert s.isascii() == False
    s = "\t\n\r"
    assert s.isascii() == True
    s = "123 45 6"
    assert s.isascii() == True

def check():
    is_lower()
    is_upper()
    is_decimal()
    is_ascii()

check()
