from string import ascii_lowercase, ascii_letters

def test_string():
    assert ascii_lowercase == 'abcdefghijklmnopqrstuvwxyz'
    assert ascii_letters == 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

    print(ascii_lowercase)