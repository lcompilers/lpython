#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <sstream>

#include <libasr/exception.h>
#include <libasr/utils.h>
#include <libasr/string_utils.h>

namespace LCompilers {

std::string get_unique_ID() {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(0, 61);
    const std::string v =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string res;
    for (int i = 0; i < 22; i++) {
        res += v[dist(rng)];
    }
    return res;
}

bool read_file(const std::string &filename, std::string &text)
{
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary
            | std::ios::ate);

    std::ifstream::pos_type filesize = ifs.tellg();
    if (filesize < 0) return false;

    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(filesize);
    ifs.read(&bytes[0], filesize);

    text = std::string(&bytes[0], filesize);
    return true;
}

bool present(Vec<char*> &v, const char* name) {
    for (auto &a : v) {
        if (std::string(a) == std::string(name)) {
            return true;
        }
    }
    return false;
}

bool present(char** const v, size_t n, const std::string name) {
    for (size_t i = 0; i < n; i++) {
        if (std::string(v[i]) == name) {
            return true;
        }
    }
    return false;
}

int visualize_json(std::string &astr_data_json, LCompilers::Platform os) {
    using namespace LCompilers;
    std::hash<std::string> hasher;
    std::string file_name = "visualize" + std::to_string(hasher(astr_data_json)) + ".html";
    std::ofstream out;
    out.open(file_name);
    out << LCompilers::generate_visualize_html(astr_data_json);
    out.close();
    std::string open_cmd = "";
    switch (os) {
        case Linux: open_cmd = "xdg-open"; break;
        case Windows: open_cmd = "start"; break;
        case macOS_Intel:
        case macOS_ARM: open_cmd = "open"; break;
        default:
            std::cerr << "Unsupported Platform " << pf2s(os) <<std::endl;
            std::cerr << "Please open file " << file_name << " manually" <<std::endl;
            return 11;
    }
    std::string cmd = open_cmd + " " + file_name;
    int err = system(cmd.data());
    if (err) {
        std::cout << "The command '" + cmd + "' failed." << std::endl;
        return 11;
    }
    return 0;
}

std::string generate_visualize_html(std::string &astr_data_json) {
    std::stringstream out;
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
    return out.str();
}

std::string pf2s(Platform p) {
    switch (p) {
        case (Platform::Linux) : return "Linux";
        case (Platform::macOS_Intel) : return "macOS Intel";
        case (Platform::macOS_ARM) : return "macOS ARM";
        case (Platform::Windows) : return "Windows";
        case (Platform::FreeBSD) : return "FreeBSD";
        case (Platform::OpenBSD) : return "OpenBSD";
    }
    return "Unsupported Platform";
}

Platform get_platform()
{
#if defined(_WIN32)
    return Platform::Windows;
#elif defined(__APPLE__)
#    ifdef __aarch64__
    return Platform::macOS_ARM;
#    else
    return Platform::macOS_Intel;
#    endif
#elif defined(__FreeBSD__)
    return Platform::FreeBSD;
#elif defined(__OpenBSD__)
    return Platform::OpenBSD;
#else
    return Platform::Linux;
#endif
}

// Platform-specific initialization
// On Windows, enable colors in terminal. On other systems, do nothing.
// Return value: 0 on success, negative number on failure.
int initialize()
{
#ifdef _WIN32
    HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_stdout == INVALID_HANDLE_VALUE)
        return -1;

    DWORD mode;
    if (! GetConsoleMode(h_stdout, &mode))
        return -2;

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (! SetConsoleMode(h_stdout, mode))
        return -3;

    return 0;
#else
    return 0;
#endif
}

} // namespace LCompilers
