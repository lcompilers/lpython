#include <lfortran/asr_utils.h>

namespace LFortran {

void visit(int a, std::map<int,std::vector<int>> &deps,
        std::vector<bool> &visited, std::vector<int> &result) {
    visited[a] = true;
    for (auto n : deps[a]) {
        if (!visited[n]) visit(n, deps, visited, result);
    }
    result.push_back(a);
}

std::vector<int> order_deps(std::map<int, std::vector<int>> &deps) {
    std::vector<bool> visited(deps.size(), false);
    std::vector<int> result;
    for (auto d : deps) {
        if (!visited[d.first]) visit(d.first, deps, visited, result);
    }
    return result;
}

std::vector<std::string> order_deps(std::map<std::string, std::vector<std::string>> &deps) {
    // Create a mapping string <-> int
    std::vector<std::string> int2string;
    std::map<std::string, int> string2int;
    for (auto d : deps) {
        if (string2int.find(d.first) == string2int.end()) {
            string2int[d.first] = int2string.size();
            int2string.push_back(d.first);
        }
        for (auto n : d.second) {
            if (string2int.find(n) == string2int.end()) {
                string2int[n] = int2string.size();
                int2string.push_back(n);
            }
        }
    }

    // Transform dep -> dep_int
    std::map<int, std::vector<int>> deps_int;
    for (auto d : deps) {
        for (auto n : d.second) {
            deps_int[string2int[d.first]].push_back(string2int[n]);
        }
    }

    // Compute ordering
    std::vector<int> result_int = order_deps(deps_int);

    // Transform result_int -> result
    std::vector<std::string> result;
    for (auto n : result_int) {
        result.push_back(int2string[n]);
    }

    return result;
}

} // namespace LFortran
