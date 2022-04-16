#!/usr/bin/env python

import argparse
from mimetypes import init
from pprint import pprint
import subprocess
import os
import toml
import re
from timeit import default_timer as clock
import ast

# when you add option to implemented_benchmarks you should also add class to get values from cpython and python.
implemented_benchmarks = [
    "parser"
]

class Parser:
    @classmethod
    def get_lpython_result(cls, file_path):
        lpython_run = subprocess.run(["lpython", "--new-parser", "--time-report", file_path], capture_output=True)
        try:
            parsing_value = re.search(r"\bparsing:.*ms\b", lpython_run.stdout.decode("utf-8"))[0]
            parsing_value = parsing_value.replace("parsing:", '')
            parsing_value = parsing_value.replace('ms', '')
            parsing_value = float(parsing_value)
        except:
            parsing_value = None

        return parsing_value

    @classmethod
    def get_cpython_result(cls, file_path):
        input = open(file_path).read()
        t1 = clock()
        print(t1)
        a = ast.parse(input, type_comments=True)
        t2 = clock()
        return float(t2 - t1) * 1000


class Graph:
    def __init__(self, plots) -> None:
        report_file = open(os.path.join('benchmarks', 'report_file.dat'), 'w')
        for plot in plots:
            report_file.write(f"{plot[0]},{plot[1]},{plot[2]}\n")

    def show(self):
        termgraph_command = subprocess.run(['termgraph', os.path.join('benchmarks', 'report_file.dat')],
                                           capture_output=True)
        print(termgraph_command.stdout.decode('utf-8'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Lpython benchmark")
    parser.add_argument("-n", "--numerical", action="store_true", help="show results as nnumerical values")
    parser.add_argument("-p", "--plots", action="store_true", help="show results as graph of plots")
    parser.add_argument("-c", "--compare", action="store", nargs='+', help=f"What you wnat to compare for now we have{implemented_benchmarks} ")
    args = parser.parse_args()
    show_graph = args.plots and True
    show_numerical = args.numerical and True
    compare_list = args.compare
    if compare_list == None:
        compare_list = implemented_benchmarks[:]

    os.environ["PATH"] = os.path.join(os.getcwd(), "src", "bin") + os.pathsep + os.environ["PATH"]
    files = toml.load("./benchmarks/benchmarks.toml")
    comparsing_map: dict = {}
    for option in implemented_benchmarks:
        comparsing_map[option] = []

    for file in files['benchmark']:
        for option in implemented_benchmarks:
            if option in file.keys():
                comparsing_map[option].append(file["filename"])

    for compare_option in comparsing_map.items():
        if compare_option == []:
            continue
        if compare_option[0] == 'parser':
            compare_result = []
            for filename in compare_option[1]:
                filename = os.path.join("benchmarks", filename)
                cpython = Parser.get_cpython_result(filename)
                lpython = Parser.get_lpython_result(filename)
                if cpython != None and lpython != None:
                    compare_result.append((filename, lpython, cpython))
            pprint(compare_result)
            graph = Graph(compare_result)
            graph.show()

