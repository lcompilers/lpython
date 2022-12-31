from ltypes import i32

def exit(error_code: i32):
    """
    Exits the program with an error code `error_code`.
    """

    quit(error_code)

def _lpython_argv() -> list[str]:
    # TODO: use argv as a global `list[str]` variable
    """
    Gets the list of command line arguments
    """
    argc: i32 = _lpython_get_argc()
    argv: list[str]
    i: i32
    for i in range(argc):
        argv.append(_lpython_get_argv(i))
    return argv

@ccall
def _lpython_get_argc() -> i32:
    pass

@ccall
def _lpython_get_argv(index: i32) -> str:
    pass
