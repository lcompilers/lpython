import argparse
import subprocess
import os
import toml
import re
from timeit import default_timer as clock
import ast
from tabulate import tabulate

# when you add option to implemented_benchmarks you must also add class to get values from cpython and lpython.
implemented_benchmarks = [
    "parser"
]


class Parser:
    @classmethod
    def get_lpython_result(cls, file_path):
        lpython_run = subprocess.Popen("lpython --new-parser --time-report " + file_path, shell=True,
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        try:
            stdout = lpython_run.communicate()[0].decode('utf-8')
            parsing_value = re.search(r"\bParsing: .*ms\b", stdout)[0]
            parsing_value = parsing_value.replace("Parsing: ", '')
            parsing_value = parsing_value.replace('ms', '')
            parsing_value = float(parsing_value)

        except:
            parsing_value = None

        return parsing_value

    @classmethod
    def get_cpython_result(cls, file_path):
        input = open(file_path).read()
        t1 = clock()
        a = ast.parse(input, type_comments=True)
        t2 = clock()
        return float(t2 - t1) * 1000


class Graph:

    def __init__(self, option_name, plots):
        self.option_name = option_name
        self.plots = plots

    def get_plotting(self):
        report_file = open(os.path.join('benchmark', 'report_file.dat'), 'w')
        for plot in self.plots:
            report_file.write(f"{plot[0]},{plot[1]},{plot[2]}\n")
        report_file.close()
        termgraph_command = subprocess.run(
            "termgraph %s --space-between" % (os.path.join('benchmark', 'report_file.dat')), shell=True,
            capture_output=True)
        res = termgraph_command.stdout.decode('utf-8')
        res = "LPython is first raw, Python is second one.\nTime in ms.\n" + res
        return res

    def get_numerical(self):
        return (tabulate(self.plots, headers=['File Name', 'LPython', 'Python']))


if __name__ == '__main__':
    os.environ["PATH"] = os.path.join(os.getcwd(), "src", "bin") + os.pathsep + os.environ["PATH"]
    app = argparse.ArgumentParser(description="Lpython benchmark")
    app.add_argument("-n", "--numerical", action="store_true", help="show results as numerical values")
    app.add_argument("-p", "--plots", action="store_true", help="show results as graph of plots")
    app.add_argument("-c", "--compare", action="store", nargs='+',
                     help=f"What you what to compare, for now we have{implemented_benchmarks}")
    args = app.parse_args()

    show_graph = args.plots or True
    show_numerical = args.numerical or True

    compare_list = args.compare
    if compare_list == None:
        compare_list = implemented_benchmarks[:]

    files = toml.load("./benchmark/benchmark.toml")
    comparsing_map: dict = {}
    for option in implemented_benchmarks:
        comparsing_map[option] = []

    for file in files['benchmark']:
        for option in implemented_benchmarks:
            if option in file.keys():
                comparsing_map[option].append(file["filename"])

    for option, files_list in comparsing_map.items():
        if files_list == []:
            pref = "\033["
            reset = f"{pref}0m"
            print(f"{pref}1;31mThere is no files for this comparision option({option}){reset}")
        else:
            compare_result = []
            if option == 'parser':

                for filename in files_list:
                    filename = os.path.join('benchmark', filename)
                    cpython = Parser.get_cpython_result(filename)
                    lpython = Parser.get_lpython_result(filename)
                    if cpython != None and lpython != None:
                        compare_result.append((filename, lpython, cpython))

            # here other comparision

            if show_graph or show_numerical:
                graph = Graph(option, compare_result)

                pref = "\033["
                reset = f"{pref}0m"
                print(f'\t\t {pref}1;32m{option} comparison {reset}')
                if show_graph:
                    print(graph.get_plotting())

                if show_numerical:
                    print(graph.get_numerical())
