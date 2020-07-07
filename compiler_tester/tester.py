import hashlib
import json
import os
import pathlib
import shutil
import subprocess
from typing import Mapping, Any, List

class RunException(Exception):
    pass

class ExecuteException(Exception):
    pass

class style:
    reset = 0
    bold = 1
    dim = 2
    italic = 3
    underline = 4
    blink = 5
    rblink = 6
    reversed = 7
    conceal = 8
    crossed = 9


class fg:
    black = 30
    red = 31
    green = 32
    yellow = 33
    blue = 34
    magenta = 35
    cyan = 36
    gray = 37
    reset = 39

def color(value):
    return "\033[" + str(int(value)) + "m";

def print_check():
    print("%s✓ %s" % (color(fg.green)+color(style.bold),
        color(fg.reset)+color(style.reset)))

def bname(base, cmd, filename):
    hstring = cmd
    if filename:
        hstring += filename
    h = hashlib.sha224(hstring.encode()).hexdigest()[:7]
    if filename:
        bname = os.path.basename(filename)
        bname, _ = os.path.splitext(bname)
        return "%s-%s-%s" % (base, bname, h)
    else:
        return "%s-%s" % (base, h)

def _compare_eq_dict(
    left: Mapping[Any, Any], right: Mapping[Any, Any], verbose: int = 0
) -> List[str]:
    explanation = []  # type: List[str]
    set_left = set(left)
    set_right = set(right)
    common = set_left.intersection(set_right)
    same = {k: left[k] for k in common if left[k] == right[k]}
    if same and verbose < 2:
        explanation += ["Omitting %s identical items" % len(same)]
    elif same:
        explanation += ["Common items:"]
        explanation += pprint.pformat(same).splitlines()
    diff = {k for k in common if left[k] != right[k]}
    if diff:
        explanation += ["Differing items:"]
        for k in diff:
            explanation += [repr({k: left[k]}) + " != " + repr({k: right[k]})]
    extra_left = set_left - set_right
    len_extra_left = len(extra_left)
    if len_extra_left:
        explanation.append(
            "Left contains %d more item%s:"
            % (len_extra_left, "" if len_extra_left == 1 else "s")
        )
        explanation.extend(
            pprint.pformat({k: left[k] for k in extra_left}).splitlines()
        )
    extra_right = set_right - set_left
    len_extra_right = len(extra_right)
    if len_extra_right:
        explanation.append(
            "Right contains %d more item%s:"
            % (len_extra_right, "" if len_extra_right == 1 else "s")
        )
        explanation.extend(
            pprint.pformat({k: right[k] for k in extra_right}).splitlines()
        )
    return explanation

def fixdir(s):
    local_dir = os.getcwd()
    return s.replace(local_dir.encode(), "$DIR".encode())

def run(basename, cmd, out_dir, infile=None):
    """
    Runs the `cmd` and collects stdout, stderr, exit code.

    The stdout, stderr and outfile are saved in the `out_dir` directory and
    all metadata is saved in a json file, whose path is returned from the
    function.

    The idea is to use this function to test the compiler by running it with
    an option to save the AST, ASR or LLVM IR or binary, and then ensure that
    the output does not change.

    Arguments:

    basename ... name of the run
    cmd ........ command to run, can use {infile} and {outfile}
    out_dir .... output directory to store output
    infile ..... optional input file. If present, it will check that it exists
                 and hash it.

    Examples:

    >>> run("cat2", "cat tests/cat.txt > {outfile}", "output", "tests/cat.txt")
    >>> run("ls4", "ls --wrong-option", "output")

    """
    assert basename is not None and basename != ""
    pathlib.Path(out_dir).mkdir(parents=True, exist_ok=True)
    if infile and not os.path.exists(infile):
        raise RunException("The input file does not exist")
    outfile = os.path.join(out_dir, basename + "." + "out")
    cmd2 = cmd.format(infile=infile, outfile=outfile)
    r = subprocess.run(cmd2, shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    if not os.path.exists(outfile):
        outfile = None
    if len(r.stdout):
        stdout_file = os.path.join(out_dir, basename + "." + "stdout")
        open(stdout_file, "wb").write(fixdir(r.stdout))
    else:
        stdout_file = None
    if len(r.stderr):
        stderr_file = os.path.join(out_dir, basename + "." + "stderr")
        open(stderr_file, "wb").write(fixdir(r.stderr))
    else:
        stderr_file = None

    if infile:
        infile_hash = hashlib.sha224(open(infile, "rb").read()).hexdigest()
    else:
        infile_hash = None
    if outfile:
        outfile_hash = hashlib.sha224(open(outfile, "rb").read()).hexdigest()
        outfile = os.path.basename(outfile)
    else:
        outfile_hash = None
    if stdout_file:
        stdout_hash = hashlib.sha224(open(stdout_file, "rb").read()).hexdigest()
        stdout_file = os.path.basename(stdout_file)
    else:
        stdout_hash = None
    if stderr_file:
        stderr_hash = hashlib.sha224(open(stderr_file, "rb").read()).hexdigest()
        stderr_file = os.path.basename(stderr_file)
    else:
        stderr_hash = None
    data = {
        "basename": basename,
        "cmd": cmd,
        "infile": infile,
        "infile_hash": infile_hash,
        "outfile": outfile,
        "outfile_hash": outfile_hash,
        "stdout": stdout_file,
        "stdout_hash": stdout_hash,
        "stderr": stderr_file,
        "stderr_hash": stderr_hash,
        "returncode": r.returncode,
    }
    json_file = os.path.join(out_dir, basename + "." + "json")
    json.dump(data, open(json_file, "w"), indent=4)
    return json_file

def run_test(basename, cmd, infile=None, update_reference=False):
    """
    Runs the test `cmd` and compare against reference results.

    The `cmd` is executed via `run` (passing in `basename` and `infile`) and
    the output is saved in the `output` directory. The generated json file is
    then compared against reference results and if it differs, the
    RunException is thrown.

    Arguments:

    basename ........... name of the run
    cmd ................ command to run, can use {infile} and {outfile}
    infile ............. optional input file. If present, it will check that
                         it exists and hash it.
    update_reference ... if True, it will copy the output into the reference
                         directory as reference results, overwriting old ones

    Examples:

    >>> run_test("cat12", "cat {infile} > {outfile}", "cat.txt",
    ...     update_reference=True)
    >>> run_test("cat12", "cat {infile} > {outfile}", "cat.txt")
    """
    s = "    * %-6s " % basename
    print(s, end="")
    basename = bname(basename, cmd, infile)
    if infile:
        infile = os.path.join("tests", infile)
    jo = run(basename, cmd, os.path.join("tests", "output"), infile=infile)
    jr = os.path.join("tests", "reference", os.path.basename(jo))
    do = json.load(open(jo))
    if update_reference:
        shutil.copyfile(jo, jr)
        for f in ["outfile", "stdout", "stderr"]:
            if do[f]:
                f_o = os.path.join(os.path.dirname(jo), do[f])
                f_r = os.path.join(os.path.dirname(jr), do[f])
                shutil.copyfile(f_o, f_r)
        return
    if not os.path.exists(jr):
        raise RunException("The reference json file '%s' does not exist" % jr)
    dr = json.load(open(jr))
    if do != dr:
        e = _compare_eq_dict(do, dr)
        print("The JSON metadata differs against reference results")
        print("Reference JSON:", jr)
        print("Output JSON:   ", jo)
        print("\n".join(e))
        if do["outfile_hash"] != dr["outfile_hash"]:
            if do["outfile_hash"] is not None and dr["outfile_hash"] is not None:
                fo = os.path.join("tests", "output", do["outfile"])
                fr = os.path.join("tests", "reference", dr["outfile"])
                if os.path.exists(fr):
                    print("Diff against: %s" % fr)
                    os.system("diff %s %s" % (fr, fo))
                else:
                    print("Reference file '%s' does not exist" % fr)
        if do["stdout_hash"] != dr["stdout_hash"]:
            if do["stdout_hash"] is not None and dr["stdout_hash"] is not None:
                fo = os.path.join("tests", "output", do["stdout"])
                fr = os.path.join("tests", "reference", dr["stdout"])
                if os.path.exists(fr):
                    print("Diff against: %s" % fr)
                    os.system("diff %s %s" % (fr, fo))
                else:
                    print("Reference file '%s' does not exist" % fr)
        if do["stderr_hash"] != dr["stderr_hash"]:
            if do["stderr_hash"] is not None and dr["stderr_hash"] is not None:
                fo = os.path.join("tests", "output", do["stderr"])
                fr = os.path.join("tests", "reference", dr["stderr"])
                if os.path.exists(fr):
                    print("Diff against: %s" % fr)
                    os.system("diff %s %s" % (fr, fo))
                else:
                    print("Reference file '%s' does not exist" % fr)
            elif do["stderr_hash"] is not None and dr["stderr_hash"] is None:
                fo = os.path.join("tests", "output", do["stderr"])
                print("No reference stderr output exists. Stderr:")
                os.system("cat %s" % fo)
        raise RunException("The reference result differs")
    print_check()
