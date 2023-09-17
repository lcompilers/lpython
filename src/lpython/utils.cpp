#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif

#include <fstream>

#include <bin/tpl/whereami/whereami.h>

#include <libasr/exception.h>
#include <lpython/utils.h>
#include <libasr/string_utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
    #include <unistd.h>
#endif

#ifdef _WIN32
    #define stat _stat
    #if !defined S_ISDIR
        #define S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
    #endif
#endif

namespace LCompilers::LPython {

void get_executable_path(std::string &executable_path, int &dirname_length)
{
#ifdef HAVE_WHEREAMI
    int length;

    length = wai_getExecutablePath(NULL, 0, &dirname_length);
    if (length > 0) {
        std::string path(length+1, '\0');
        wai_getExecutablePath(&path[0], length, &dirname_length);
        executable_path = path;
        if (executable_path[executable_path.size()-1] == '\0') {
            executable_path = executable_path.substr(0,executable_path.size()-1);
        }
    } else {
        throw LCompilersException("Cannot determine executable path.");
    }
#else
    executable_path = "src/bin/lpython.js";
    dirname_length = 7;
#endif
}

std::string get_runtime_library_dir()
{
#ifdef HAVE_BUILD_TO_WASM
    return "asset_dir";
#endif
    char *env_p = std::getenv("LFORTRAN_RUNTIME_LIBRARY_DIR");
    if (env_p) return env_p;

    std::string path;
    int dirname_length;
    get_executable_path(path, dirname_length);
    std::string dirname = path.substr(0,dirname_length);
    if (   endswith(dirname, "src/bin")
        || endswith(dirname, "src\\bin")
        || endswith(dirname, "SRC\\BIN")) {
        // Development version
        return dirname + "/../runtime";
    } else if (endswith(dirname, "src/lpython/tests") ||
               endswith(to_lower(dirname), "src\\lpython\\tests")) {
        // CTest Tests
        return dirname + "/../../runtime";
    } else {
        // Installed version
        return dirname + "/../share/lpython/lib";
    }
}

std::string get_runtime_library_header_dir()
{
    char *env_p = std::getenv("LFORTRAN_RUNTIME_LIBRARY_HEADER_DIR");
    if (env_p) return env_p;

    // The header file is in src/libasr/runtime for development, but in impure
    // in installed version
    std::string path;
    int dirname_length;
    get_executable_path(path, dirname_length);
    std::string dirname = path.substr(0,dirname_length);
    if (   endswith(dirname, "src/bin")
        || endswith(dirname, "src\\bin")
        || endswith(dirname, "SRC\\BIN")) {
        // Development version
        return dirname + "/../libasr/runtime";
    } else if (endswith(dirname, "src/lpython/tests") ||
               endswith(to_lower(dirname), "src\\lpython\\tests")) {
        // CTest Tests
        return dirname + "/../../libasr/runtime";
    } else {
        // Installed version
        return dirname + "/../share/lpython/lib/impure";
    }

    return path;
}

bool is_directory(std::string path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        if (S_ISDIR(buffer.st_mode)) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool path_exists(std::string path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return true;
    } else {
        return false;
    }
}

// Decodes the exit status code of the process (in Unix)
// See `WEXITSTATUS` for more information.
// https://stackoverflow.com/a/27117435/15913193
// https://linux.die.net/man/3/system
int32_t get_exit_status(int32_t err) {
    return (((err) >> 8) & 0x000000ff);
}

std::string generate_visualize_html(std::string &astr_data_json) {
    std::hash<std::string> hasher;
    std::ofstream out;
    std::string file_name = "visualize" + std::to_string(hasher(astr_data_json)) + ".html";
    out.open(file_name);
    out << R"(<!DOCTYPE html>
<html>
<head>
    <title>LCompilers AST/R Visualization</title>
    <script crossorigin src="https://unpkg.com/react@18/umd/react.production.min.js"></script>
    <script crossorigin src="https://unpkg.com/react-dom@18/umd/react-dom.production.min.js"></script>

    <script src="https://unpkg.com/@babel/standalone/babel.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/react-flow-renderer/10.3.17/umd/index.js"></script>
    <script src="https://dagrejs.github.io/project/dagre/latest/dagre.min.js"></script>
    <script> )";
    out << "var astr_data = " << astr_data_json << "; </script>\n";
    out << R"(</head>

<body style="margin: 0px;">
    <script type="text/babel" data-type="module">
function TreeNode({ node }) {
    if (node.literals.length === 0) return <p><b>{node.node}</b></p>;
    return (
        <div>
            <p><b>{node.node}</b></p>
            <div style={{ backgroundColor: "#FBBD23", padding: "2px" }}>
                {
                    node.literals.map((val, idx) => <p style={{ margin: "0px", padding: "1px" }} key={idx}>{val[0]}: {val[1]}</p>)
                }
            </div>
        </div>
    );
}

const getLayoutedElements = (nodes, edges, direction = 'TB') => {
    const nodeWidth = 180;
    const isHorizontal = direction === 'LR';

    const dagreGraph = new dagre.graphlib.Graph();
    dagreGraph.setDefaultEdgeLabel(() => ({}));
    dagreGraph.setGraph({ rankdir: direction });

    nodes.forEach(node => dagreGraph.setNode(node.id, { width: nodeWidth, height: node.nodeHeight }));
    edges.forEach(edge => dagreGraph.setEdge(edge.source, edge.target));

    dagre.layout(dagreGraph);

    nodes.forEach((node) => {
        const nodeWithPosition = dagreGraph.node(node.id);
        node.targetPosition = isHorizontal ? 'left' : 'top';
        node.sourcePosition = isHorizontal ? 'right' : 'bottom';
        // Shifting the dagre node position (anchor=center center) to the top left
        // so it matches the React Flow node anchor point (top left).
        node.position = {
            x: nodeWithPosition.x - nodeWidth / 2,
            y: nodeWithPosition.y - node.nodeHeight / 2,
        };
        return node;
    });

    return [nodes, edges];
};

class Graph {
    constructor() {
        this.nodes = [];
        this.edges = [];
        this.idx = 1;
        return this;
    }

    createNode(cur_node) {
        cur_node.idx = this.idx++;
        cur_node.literals = [];
        let obj = cur_node.fields;
        for (let prop in obj) {
            let neigh = obj[prop];
            if (typeof neigh === 'object') {
                if (neigh.hasOwnProperty("node")) {
                    this.createEdge(cur_node.idx, neigh, prop);
                } else {
                    if (neigh.length > 0) {
                        for (let i in neigh) {
                            let arrayElement = neigh[i];
                            if (typeof arrayElement === 'object') {
                                if (arrayElement.hasOwnProperty("node")) {
                                    this.createEdge(cur_node.idx, arrayElement, `${prop}[${i}]`);
                                } else {
                                    console.log("ERROR: Unexpected 2D Array found");
                                }
                            } else {
                                cur_node.literals.push([`${prop}[${i}]`, `${arrayElement}`]);
                            }
                        }
                    } else {
                        // 0 length array, show as literal
                        cur_node.literals.push([prop, "[]"]);
                    }
                }
            } else {
                cur_node.literals.push([prop, `${neigh}`]);
            }
        }

        this.nodes.push({ id: `${cur_node.idx}`, data: { label: <TreeNode node={cur_node} /> }, nodeHeight: 70 + 20 * (cur_node.literals.length) });
    }

    createEdge(parent_idx, cur_node, edge_label) {
        this.edges.push({
            id: `${parent_idx}-${this.idx}`,
            source: `${parent_idx}`,
            target: `${this.idx}`,
            label: edge_label,
            labelStyle: { fontWeight: 700 },
            labelBgPadding: [8, 4],
            labelBgStyle: { fill: '#FBBD23' },
        });
        this.createNode(cur_node);
    }
}

function Flow({ nodes, edges }) {
    return (
        <div style={{ height: '100vh' }}>
            <ReactFlow.default
                defaultNodes={nodes}
                defaultEdges={edges}
                style={{ backgroundColor: '#e5e7eb' }}
            >
                <ReactFlow.Background />
                <ReactFlow.Controls />
                <ReactFlow.MiniMap />
            </ReactFlow.default>
        </div>
    );
}

function MyApp() {
    var g = new Graph();
    g.createNode(astr_data);
    var [layoutedNodes, layoutedEdges] = getLayoutedElements(g.nodes, g.edges);
    return (<Flow nodes={layoutedNodes} edges={layoutedEdges} />);
}

ReactDOM.render(<MyApp />, document.body);
    </script>
</body>

</html>)";
    return file_name;
}

}
