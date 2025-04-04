import platform


def get_cpython_version():    
    return platform.python_version()


def get_modified_dict(d):
    d["LFortran"] = 100
    return d


def get_modified_list(l):
    l.append("LFortran")
    return l


def get_modified_tuple(t):
    return (t[0], t[1], t[0] + t[1])


def get_modified_set(s):
    s.add(100)
    return s
