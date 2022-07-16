#!/usr/bin/env python
import argparse
import ast
from lib2to3.pgen2.pgen import generate_grammar
from lib2to3.pytree import Node
import os
import re
import subprocess
from timeit import default_timer as clock
from numpy import PINF, true_divide
import sys
from io import StringIO
import toml
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

        except Exception as e :
            parsing_value = None
            print(e)

        return parsing_value

    @classmethod
    def get_cpython_result(cls, file_path):
        input = open(file_path).read()
        t1 = clock()
        a = ast.parse(input, type_comments=True)
        t2 = clock()
        return float(t2 - t1) * 1000

class Genrator:
    @classmethod
    def generate_code(cls, generating_script_file):
        old_stdout = sys.stdout
        redirected_output = sys.stdout = StringIO()
        exec(open(generating_script_file).read())
        sys.stdout = old_stdout
        generated_code_str = redirected_output.getvalue()
        generated_code_file_name = os.path.join('benchmark', 'generated_code', os.path.split(generating_script_file)[-1])
        generated_code_file = open(generated_code_file_name, 'w')
        generated_code_file.write(generated_code_str)
        generated_code_file.close()
        return generated_code_file_name



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
    app.add_argument("-n", "--numerical", action="store_true", help="show results as numerical table")
    app.add_argument("-p", "--plots", action="store_true", help="show results as graph of pars")
    app.add_argument("-c", "--compare", action="store", nargs='+',
                     help=f"What stages you want to compare, for now we have{implemented_benchmarks}")
    args = app.parse_args()

    show_graph = args.plots or True
    show_numerical = args.numerical or True
    compare_stage_iwant = args.compare
    if compare_stage_iwant == None:
        compare_stage_iwant = implemented_benchmarks[:]

    files = toml.load("./benchmark/benchmark.toml")
    comparsing_map: dict = {}
    for option in implemented_benchmarks:
        comparsing_map[option] = dict()
        comparsing_map[option][0] = [] #basic files 
        comparsing_map[option][1] = [] #genertaed files


    for file in files['benchmark']:
        for option in compare_stage_iwant:
            if option in file.keys():
                comparsing_map[option][int(('generate' in file.keys()) and file['generate'] == True)].append(file["filename"])

    for option, files_list in comparsing_map.items():
        basic_files_list = files_list[0]
        generated_files_list = files_list[1]
        if generated_files_list == [] and basic_files_list == []:
            pref = "\033["
            reset = f"{pref}0m"
            print(f"{pref}1;31mThere is no files for this comparision option({option}){reset}")
        else:
            compare_result = []

            if option == 'parser':
                for filename in basic_files_list:
                    basic_code_file = os.path.join('benchmark', filename)
                    cpython = Parser.get_cpython_result(basic_code_file)
                    lpython = Parser.get_lpython_result(basic_code_file)
                    if cpython != None and lpython != None:
                        compare_result.append((filename, lpython, cpython))
                
                for filename in generated_files_list:
                    generating_script_file = os.path.join( 'benchmark', 'generating_scripts', filename)
                    generated_code_file = None
                    try:
                        generated_code_file = Genrator.generate_code(generating_script_file)
                    except Exception as e:
                        print(e)
                    cpython = Parser.get_cpython_result(generated_code_file)
                    lpython = Parser.get_lpython_result(generated_code_file)
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
