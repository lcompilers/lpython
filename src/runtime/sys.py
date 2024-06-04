from lpython import i32

def exit(error_code: i32):
    """
    Exits the program with an error code `error_code`.
    """
    quit(error_code)

# >----------------------------------- argv ----------------------------------->
@ccall
def _lpython_get_argc() -> i32:
    pass

@ccall
def _lpython_get_argv(index: i32) -> str:
    pass

def _lpython_argv() -> list[str]:
    """
    Gets the list of command line arguments
    """
    argc: i32 = _lpython_get_argc()
    argv: list[str] = []
    i: i32
    for i in range(argc):
        argv.append(_lpython_get_argv(i))
    return argv

argv: list[str] = _lpython_argv()

# <----------------------------------- argv -----------------------------------<
