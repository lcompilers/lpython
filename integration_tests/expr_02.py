def main0():
    a: bool
    b: bool
    a = False
    b = True
    a = a and b
    b = a or True
    a = a or b

main0()
# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
