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
    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace LFortran
