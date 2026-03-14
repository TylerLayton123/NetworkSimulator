#include "algorithmpanel.h"
#include "netsim_classes.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
#include <QStackedWidget>
#include <QFont>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QQueue>
#include <QStack>
#include <QMap>
#include <QSet>
#include <functional>
#include <limits>
#include <algorithm>
#include <climits>

// ---------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------
AlgorithmPanel::AlgorithmPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

// ---------------------------------------------------------------
// Data
// ---------------------------------------------------------------
void AlgorithmPanel::setData(const QList<NetworkNode*>& nodes,
                              const QList<NetworkEdge*>& edges)
{
    m_nodes = nodes;
    m_edges = edges;
    if (m_sourceInfo) {
        NetworkNode* src = sourceOrFirst();
        m_sourceInfo->setText(src
            ? QString("  Source: %1   (select a node on the canvas to change)").arg(src->label())
            : "  Source: none");
    }
}

void AlgorithmPanel::setSourceNode(NetworkNode* node)
{
    m_sourceNode = node;
    if (m_sourceInfo && node)
        m_sourceInfo->setText(
            QString("  Source: %1   (select a node on the canvas to change)").arg(node->label()));
}

// ---------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------
void AlgorithmPanel::buildUI()
{
    setStyleSheet(
        "AlgorithmPanel {"
        "  border: 1px solid #b0b8c8;"
        "  background-color: #f8f9fc;"
        "}"
    );

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Title bar ────────────────────────────────────────────────
    auto* titleBar = new QWidget;
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet(
        "QWidget#titleBar { border-bottom: 1px solid #b0b8c8; background: transparent; }"
        "QLabel { border: none; }"
    );

    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 8, 0);
    titleLayout->setSpacing(6);

    auto* titleLbl = new QLabel("Algorithms");
    QFont tf = titleLbl->font();
    tf.setBold(true);
    tf.setPointSize(11);
    titleLbl->setFont(tf);
    titleLayout->addWidget(titleLbl);
    titleLayout->addStretch(1);

    m_searchBtn  = new QPushButton("Search");
    m_metricsBtn = new QPushButton("Metrics");
    m_searchBtn->setCheckable(true);
    m_metricsBtn->setCheckable(true);
    m_searchBtn->setAutoExclusive(true);
    m_metricsBtn->setAutoExclusive(true);
    titleLayout->addWidget(m_searchBtn);
    titleLayout->addWidget(m_metricsBtn);

    connect(m_searchBtn,  &QPushButton::clicked, this, &AlgorithmPanel::showSearchPage);
    connect(m_metricsBtn, &QPushButton::clicked, this, &AlgorithmPanel::showMetricsPage);

    // ── Column header ─────────────────────────────────────────────
    auto* colHeader = new QWidget;
    colHeader->setObjectName("colHeader");
    colHeader->setFixedHeight(22);
    colHeader->setStyleSheet(
        "QWidget#colHeader { border-bottom: 1px solid #b0b8c8; background: transparent; }"
        "QLabel { border: none; }"
    );
    auto* colLayout = new QHBoxLayout(colHeader);
    colLayout->setContentsMargins(8, 0, 8, 0);
    colLayout->setSpacing(8);

    auto mkHdr = [](const QString& t, int w = -1) {
        auto* l = new QLabel(t);
        QFont f = l->font(); f.setPointSize(10); l->setFont(f);
        if (w > 0) l->setFixedWidth(w);
        return l;
    };
    colLayout->addWidget(mkHdr("Algorithm", 130));
    colLayout->addWidget(mkHdr("Description"), 1);
    colLayout->addWidget(mkHdr("", 52));

    // ── Stacked pages ─────────────────────────────────────────────
    m_stack = new QStackedWidget;

    const QList<QPair<QString,QString>> searchAlgos = {
        { "bfs",        "Breadth-first traversal from a source node" },
        { "dfs",        "Depth-first traversal from a source node" },
        { "dijkstra",   "Shortest paths — uses edge labels as weights" },
        // { "cycle",      "Detect cycles via union-find" },
        { "components", "Count and list all connected components" },
        { "topo",       "Topological sort (DAGs only)" },
    };

    const QList<QPair<QString,QString>> visualAlgos = {
        // { "mst",        "Minimum spanning tree (Kruskal's)" },
        // { "degree",     "Degree distribution and graph metrics" },
        // { "bipartite",  "Check if the graph is bipartite (2-colorable)" },
        // { "density",    "Edge density, clustering coefficient, summary" },
    };

    m_stack->addWidget(buildAlgoPage(searchAlgos));   // index 0
    m_stack->addWidget(buildAlgoPage(visualAlgos));  // index 1

    // ── Source info bar ───────────────────────────────────────────
    m_sourceInfo = new QLabel("  Source: none");
    m_sourceInfo->setFixedHeight(18);
    m_sourceInfo->setStyleSheet(
        "background-color: #e8ecf5; color: #3a4a6a; font-size: 10px;"
        "border-top: 1px solid #b0b8c8; border-bottom: 1px solid #b0b8c8;"
    );

    // ── Output area ───────────────────────────────────────────────
    m_output = new QTextEdit;
    m_output->setReadOnly(true);
    m_output->setMinimumHeight(80);
    m_output->setMaximumHeight(130);
    m_output->setStyleSheet(
        "QTextEdit {"
        "  background-color: white; color: black;"
        "  border: none; padding: 6px 8px;"
        "}"
        "QScrollBar:vertical { background: white; width:7px; }"
        "QScrollBar::handle:vertical { background: #b0b8c8; border-radius:3px; }"
    );
    m_output->setPlaceholderText("Run an algorithm to see output here…");

    root->addWidget(titleBar);
    root->addWidget(colHeader);
    root->addWidget(m_stack, 1);
    root->addWidget(m_sourceInfo);
    root->addWidget(m_output);

    showSearchPage();
}

QWidget* AlgorithmPanel::buildAlgoPage(const QList<QPair<QString,QString>>& algos)
{
    static const QMap<QString,QString> names = {
        { "bfs",        "BFS"           },
        { "dfs",        "DFS"           },
        { "dijkstra",   "Dijkstra"      },
        // { "cycle",      "Cycle Detect"  },
        { "components", "Components"    },
        { "topo",       "Topo Sort"     },
        // { "mst",        "MST (Kruskal)" },
        // { "degree",     "Degree Stats"  },
        // { "bipartite",  "Bipartite"     },
        // { "density",    "Density"       },
    };

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget;
    auto* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    bool alt = false;
    for (const auto& pair : algos) {
        const QString& id   = pair.first;
        const QString& desc = pair.second;

        auto* row = new QWidget;
        row->setFixedHeight(30);
        row->setStyleSheet(QString(
            "QWidget { background-color: %1; border-bottom: 1px solid #d0d4e0; }"
            "QLabel  { border: none; background: transparent; }"
            "QPushButton { border: none; }"
        ).arg(alt ? "#edf0f8" : "#f8f9fc"));
        alt = !alt;

        auto* rl = new QHBoxLayout(row);
        rl->setContentsMargins(8, 0, 8, 0);
        rl->setSpacing(8);

        auto* nameLbl = new QLabel(names.value(id, id));
        nameLbl->setFixedWidth(130);
        QFont nf = nameLbl->font(); nf.setBold(true); nf.setPointSize(10);
        nameLbl->setFont(nf);
        nameLbl->setStyleSheet("color: #1a2a4a;");

        auto* descLbl = new QLabel(desc);
        descLbl->setStyleSheet("color: #4a5a7a; font-size: 10px;");
        descLbl->setWordWrap(false);

        auto* runBtn = new QPushButton("Run");
        runBtn->setFixedSize(50, 20);
        runBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #3a6ab0; color: white;"
            "  border: none; border-radius: 3px; font-size: 10px;"
            "}"
            "QPushButton:hover   { background-color: #4a7ac0; }"
            "QPushButton:pressed { background-color: #2a5aa0; }"
        );

        connect(runBtn, &QPushButton::clicked, this, [this, id]() {
            runAlgorithm(id);
        });

        rl->addWidget(nameLbl);
        rl->addWidget(descLbl, 1);
        rl->addWidget(runBtn);
        vbox->addWidget(row);
    }

    vbox->addStretch(1);
    scroll->setWidget(container);
    return scroll;
}

// ---------------------------------------------------------------
// Node-picker dialog
// ---------------------------------------------------------------
bool AlgorithmPanel::askParams(const QString& algoName, bool needsSource,
                                bool needsTarget, AlgoParams& out)
{
    if (m_nodes.isEmpty()) return false;

    QDialog dlg(this);
    dlg.setWindowTitle(algoName);
    dlg.setMinimumWidth(300);

    auto* form   = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);

    // ── Source combo ──────────────────────────────────────────────
    QComboBox* sourceCbo = nullptr;
    if (needsSource) {
        sourceCbo = new QComboBox;
        for (NetworkNode* n : m_nodes)
            sourceCbo->addItem(n->label(), QVariant::fromValue(static_cast<void*>(n)));

        // Pre-select the current source node if valid
        NetworkNode* presel = sourceOrFirst();
        if (presel) {
            int idx = m_nodes.indexOf(presel);
            if (idx >= 0) sourceCbo->setCurrentIndex(idx);
        }
        form->addRow("Source node:", sourceCbo);
    }

    // ── Target combo ─────────────────────────────────────────────
    QComboBox* targetCbo = nullptr;
    if (needsTarget) {
        targetCbo = new QComboBox;
        targetCbo->addItem("(none — show all)", QVariant::fromValue(static_cast<void*>(nullptr)));
        for (NetworkNode* n : m_nodes)
            targetCbo->addItem(n->label(), QVariant::fromValue(static_cast<void*>(n)));
        form->addRow("Target node (optional):", targetCbo);
    }

    // ── Buttons ───────────────────────────────────────────────────
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    if (sourceCbo) {
        void* ptr = sourceCbo->currentData().value<void*>();
        out.source = static_cast<NetworkNode*>(ptr);
    }

    if (targetCbo) {
        void* ptr = targetCbo->currentData().value<void*>();
        out.target = static_cast<NetworkNode*>(ptr); // may be nullptr
    }

    return true;
}

// ---------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------
void AlgorithmPanel::runAlgorithm(const QString& id)
{
    if (m_nodes.isEmpty()) {
        printResult("Error", "No graph loaded — add some nodes first.");
        return;
    }

    QString   title, result;
    AlgoParams p;

    // Algorithms that need a source node + optional target
    if (id == "bfs" || id == "dfs" || id == "dijkstra") {
        bool needsTarget = (id == "bfs" || id == "dfs" || id == "dijkstra");
        if (!askParams(id.toUpper(), true, needsTarget, p)) return;

        if      (id == "bfs")      { title = "BFS";      result = algoBFS(p.source, p.target); }
        else if (id == "dfs")      { title = "DFS";      result = algoDFS(p.source, p.target); }
        else if (id == "dijkstra") { title = "Dijkstra"; result = algoDijkstra(p.source, p.target); }
    }
    // Everything else runs without a dialog
    // else if (id == "cycle")      { title = "Cycle Detection";      result = algoCycleDetection(); }
    else if (id == "components") { title = "Connected Components"; result = algoConnectedComponents(); }
    else if (id == "topo")       { title = "Topological Sort";     result = algoTopoSort(); }
    // else if (id == "mst")        { title = "MST (Kruskal's)";      result = algoMST(); }
    // else if (id == "degree")     { title = "Degree Statistics";    result = algoDegreeStats(); }
    // else if (id == "bipartite")  { title = "Bipartite Check";      result = algoBipartite(); }
    // else if (id == "density")    { title = "Graph Density";        result = algoGraphDensity(); }

    printResult(title, result);
}

void AlgorithmPanel::printResult(const QString& title, const QString& body)
{
    if (m_output)
        m_output->setPlainText(QString("=== %1 ===\n%2").arg(title, body));
}

// ---------------------------------------------------------------
// Page switching
// ---------------------------------------------------------------
void AlgorithmPanel::showSearchPage()
{
    if (m_stack) m_stack->setCurrentIndex(0);
    if (m_searchBtn)  m_searchBtn->setChecked(true);
    if (m_metricsBtn) m_metricsBtn->setChecked(false);
}

void AlgorithmPanel::showMetricsPage()
{
    if (m_stack) m_stack->setCurrentIndex(1);
    if (m_metricsBtn) m_metricsBtn->setChecked(true);
    if (m_searchBtn)  m_searchBtn->setChecked(false);
}

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------
NetworkNode* AlgorithmPanel::sourceOrFirst() const
{
    if (m_sourceNode && m_nodes.contains(m_sourceNode)) return m_sourceNode;
    return m_nodes.isEmpty() ? nullptr : m_nodes.first();
}

double AlgorithmPanel::edgeWeight(NetworkEdge* e) const
{
    bool ok;
    double w = e->getLabel().toDouble(&ok);
    return ok ? w : 1.0;
}

NetworkNode* AlgorithmPanel::neighbour(NetworkEdge* edge, NetworkNode* from) const
{
    return (edge->sourceNode() == from) ? edge->destNode() : edge->sourceNode();
}

// ---------------------------------------------------------------
// BFS  (source required, target optional)
// ---------------------------------------------------------------
QString AlgorithmPanel::algoBFS(NetworkNode* source, NetworkNode* target)
{
    if (!source) return "No source node.";

    QMap<NetworkNode*, bool>       visited;
    QMap<NetworkNode*, NetworkNode*> prev;
    QQueue<NetworkNode*> queue;
    QStringList order;
    bool foundTarget = false;

    visited[source] = true;
    prev[source]    = nullptr;
    queue.enqueue(source);

    while (!queue.isEmpty()) {
        NetworkNode* cur = queue.dequeue();
        order << cur->label();

        if (target && cur == target) { foundTarget = true; break; }

        for (NetworkEdge* e : cur->getEdgeList()) {
            NetworkNode* nb = neighbour(e, cur);
            if (!visited.contains(nb)) {
                visited[nb] = true;
                prev[nb]    = cur;
                queue.enqueue(nb);
            }
        }
    }

    QStringList lines;
    lines << QString("Source     : %1").arg(source->label());

    if (target) {
        lines << QString("Target     : %1").arg(target->label());
        if (foundTarget) {
            // Reconstruct path
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Found after: %1 step(s)").arg(path.size() - 1);
            lines << QString("Path       : %1").arg(path.join(" → "));
        } else {
            lines << "Target not reachable from source.";
        }
        lines << "";
    }

    lines << QString("Visit order: %1").arg(order.join(" → "));
    lines << QString("Visited    : %1 / %2").arg(visited.size()).arg(m_nodes.size());

    int unreached = m_nodes.size() - visited.size();
    if (unreached > 0)
        lines << QString("Unreached  : %1 node(s) (disconnected)").arg(unreached);

    return lines.join("\n");
}

// ---------------------------------------------------------------
// DFS  (source required, target optional)
// ---------------------------------------------------------------
QString AlgorithmPanel::algoDFS(NetworkNode* source, NetworkNode* target)
{
    if (!source) return "No source node.";

    QSet<NetworkNode*>               visited;
    QMap<NetworkNode*, NetworkNode*> prev;
    QStack<NetworkNode*>             stack;
    QStringList                      order;
    bool foundTarget = false;

    stack.push(source);
    prev[source] = nullptr;

    while (!stack.isEmpty()) {
        NetworkNode* cur = stack.pop();
        if (visited.contains(cur)) continue;
        visited.insert(cur);
        order << cur->label();

        if (target && cur == target) { foundTarget = true; break; }

        QList<NetworkEdge*> es = cur->getEdgeList();
        for (int i = es.size() - 1; i >= 0; --i) {
            NetworkNode* nb = neighbour(es[i], cur);
            if (!visited.contains(nb)) {
                if (!prev.contains(nb)) prev[nb] = cur;
                stack.push(nb);
            }
        }
    }

    QStringList lines;
    lines << QString("Source     : %1").arg(source->label());

    if (target) {
        lines << QString("Target     : %1").arg(target->label());
        if (foundTarget) {
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Found after: %1 step(s)").arg(path.size() - 1);
            lines << QString("Path       : %1").arg(path.join(" → "));
        } else {
            lines << "Target not reachable from source.";
        }
        lines << "";
    }

    lines << QString("Visit order: %1").arg(order.join(" → "));
    lines << QString("Visited    : %1 / %2").arg(visited.size()).arg(m_nodes.size());

    int unreached = m_nodes.size() - visited.size();
    if (unreached > 0)
        lines << QString("Unreached  : %1 node(s) (disconnected)").arg(unreached);

    return lines.join("\n");
}

// ---------------------------------------------------------------
// Dijkstra  (source required, target optional)
// ---------------------------------------------------------------
QString AlgorithmPanel::algoDijkstra(NetworkNode* source, NetworkNode* target)
{
    if (!source)              return "No source node.";
    if (m_nodes.size() < 2)  return "Need at least 2 nodes.";

    bool allNumeric = true;
    for (NetworkEdge* e : m_edges) {
        bool ok; e->getLabel().toDouble(&ok);
        if (!ok && !e->getLabel().isEmpty()) { allNumeric = false; break; }
    }

    const double INF = std::numeric_limits<double>::infinity();
    QMap<NetworkNode*, double>       dist;
    QMap<NetworkNode*, NetworkNode*> prev;
    QList<NetworkNode*> unvisited = m_nodes;
    for (NetworkNode* n : m_nodes) dist[n] = INF;
    dist[source] = 0.0;

    while (!unvisited.isEmpty()) {
        NetworkNode* u = nullptr;
        for (NetworkNode* n : unvisited) if (!u || dist[n] < dist[u]) u = n;
        if (dist[u] == INF) break;
        // Early exit if we only care about the target
        if (target && u == target) break;
        unvisited.removeOne(u);
        for (NetworkEdge* e : u->getEdgeList()) {
            NetworkNode* v = neighbour(e, u);
            if (!unvisited.contains(v)) continue;
            double alt = dist[u] + edgeWeight(e);
            if (alt < dist[v]) { dist[v] = alt; prev[v] = u; }
        }
    }

    QStringList lines;
    lines << QString("Source: %1%2")
             .arg(source->label())
             .arg(allNumeric ? "" : "  [non-numeric labels treated as weight 1]");

    // If a specific target was requested, just show that path
    if (target) {
        lines << QString("Target: %1").arg(target->label());
        lines << "";
        if (dist[target] == INF) {
            lines << "Target is unreachable from source.";
        } else {
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Distance : %1").arg(QString::number(dist[target], 'f', 2));
            lines << QString("Path     : %1").arg(path.join(" → "));
        }
        return lines.join("\n");
    }

    // No target — show full table
    lines << "" << "Node        Distance   Path" << QString(45, '-');
    for (NetworkNode* n : m_nodes) {
        if (n == source) continue;
        if (dist[n] == INF) {
            lines << QString("%-12s unreachable").arg(n->label());
        } else {
            QStringList path;
            for (NetworkNode* cur = n; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("%-12s %-10s %s")
                     .arg(n->label())
                     .arg(QString::number(dist[n], 'f', 2))
                     .arg(path.join(" → "));
        }
    }
    return lines.join("\n");
}

// ---------------------------------------------------------------
// Cycle Detection
// ---------------------------------------------------------------
QString AlgorithmPanel::algoCycleDetection()
{
    QMap<NetworkNode*, NetworkNode*> parent;
    for (NetworkNode* n : m_nodes) parent[n] = n;
    std::function<NetworkNode*(NetworkNode*)> find = [&](NetworkNode* n) -> NetworkNode* {
        if (parent[n] != n) parent[n] = find(parent[n]);
        return parent[n];
    };

    QStringList cycleEdges;
    for (NetworkEdge* e : m_edges) {
        NetworkNode* ra = find(e->sourceNode());
        NetworkNode* rb = find(e->destNode());
        if (ra == rb) {
            QString lbl = e->getLabel().isEmpty() ? "—" : e->getLabel();
            cycleEdges << QString("  %1 — %2  (label: %3)")
                          .arg(e->sourceNode()->label(), e->destNode()->label(), lbl);
        } else {
            parent[ra] = rb;
        }
    }

    if (cycleEdges.isEmpty())
        return "No cycles detected.\nThe graph is acyclic (forest / tree).";
    return QString("Cycles detected — %1 back edge(s):\n\n").arg(cycleEdges.size())
           + cycleEdges.join("\n");
}

// ---------------------------------------------------------------
// Connected Components
// ---------------------------------------------------------------
QString AlgorithmPanel::algoConnectedComponents()
{
    QMap<NetworkNode*, int> comp;
    int numComp = 0;

    for (NetworkNode* sn : m_nodes) {
        if (comp.contains(sn)) continue;
        QQueue<NetworkNode*> q;
        q.enqueue(sn);
        comp[sn] = numComp;
        while (!q.isEmpty()) {
            NetworkNode* cur = q.dequeue();
            for (NetworkEdge* e : cur->getEdgeList()) {
                NetworkNode* nb = neighbour(e, cur);
                if (!comp.contains(nb)) { comp[nb] = numComp; q.enqueue(nb); }
            }
        }
        ++numComp;
    }

    QMap<int, QStringList> groups;
    for (auto it = comp.cbegin(); it != comp.cend(); ++it)
        groups[it.value()] << it.key()->label();

    QStringList lines;
    lines << QString("%1 connected component(s):\n").arg(numComp);
    for (int i = 0; i < numComp; ++i)
        lines << QString("  [%1]  { %2 }").arg(i + 1).arg(groups[i].join(", "));
    lines << (numComp == 1 ? "\nGraph is fully connected."
                           : QString("\nGraph is disconnected (%1 components).").arg(numComp));
    return lines.join("\n");
}

// ---------------------------------------------------------------
// Topological Sort
// ---------------------------------------------------------------
QString AlgorithmPanel::algoTopoSort()
{
    QMap<NetworkNode*, int> inDeg;
    QMap<NetworkNode*, QList<NetworkNode*>> adj;
    for (NetworkNode* n : m_nodes) inDeg[n] = 0;
    for (NetworkEdge* e : m_edges) {
        adj[e->sourceNode()] << e->destNode();
        inDeg[e->destNode()]++;
    }

    QQueue<NetworkNode*> q;
    for (NetworkNode* n : m_nodes)
        if (inDeg[n] == 0) q.enqueue(n);

    QStringList order;
    while (!q.isEmpty()) {
        NetworkNode* cur = q.dequeue();
        order << cur->label();
        for (NetworkNode* nb : adj[cur])
            if (--inDeg[nb] == 0) q.enqueue(nb);
    }

    if (order.size() != m_nodes.size())
        return "Graph contains a cycle — topological sort not possible.\n"
               "(Run Cycle Detection to locate it.)";
    return QString("Topological order:\n\n%1\n\nValid linear ordering of %2 nodes.")
        .arg(order.join(" → ")).arg(order.size());
}

// ---------------------------------------------------------------
// MST (Kruskal's)
// ---------------------------------------------------------------
QString AlgorithmPanel::algoMST()
{
    if (m_nodes.size() < 2) return "Need at least 2 nodes.";
    if (m_edges.isEmpty())  return "No edges in the graph.";

    QList<NetworkEdge*> sorted = m_edges;
    std::sort(sorted.begin(), sorted.end(),
              [this](NetworkEdge* a, NetworkEdge* b) { return edgeWeight(a) < edgeWeight(b); });

    QMap<NetworkNode*, NetworkNode*> parent;
    for (NetworkNode* n : m_nodes) parent[n] = n;
    std::function<NetworkNode*(NetworkNode*)> find = [&](NetworkNode* n) -> NetworkNode* {
        if (parent[n] != n) parent[n] = find(parent[n]);
        return parent[n];
    };

    QList<NetworkEdge*> mst;
    double totalW = 0.0;
    for (NetworkEdge* e : sorted) {
        NetworkNode* ra = find(e->sourceNode());
        NetworkNode* rb = find(e->destNode());
        if (ra != rb) {
            parent[ra] = rb; mst << e; totalW += edgeWeight(e);
            if (mst.size() == m_nodes.size() - 1) break;
        }
    }

    QStringList lines;
    if (mst.size() < m_nodes.size() - 1) lines << "Warning: graph disconnected — partial MST.\n";
    lines << QString("MST edges    : %1").arg(mst.size());
    lines << QString("Total weight : %1\n").arg(totalW, 0, 'f', 2);
    lines << QString("%-22s %s").arg("Edge", "Weight") << QString(32, '-');
    for (NetworkEdge* e : mst) {
        QString pair = QString("%1 — %2").arg(e->sourceNode()->label(), e->destNode()->label());
        lines << QString("%-22s %s").arg(pair, e->getLabel().isEmpty() ? "1" : e->getLabel());
    }
    return lines.join("\n");
}

// ---------------------------------------------------------------
// Degree Statistics
// ---------------------------------------------------------------
QString AlgorithmPanel::algoDegreeStats()
{
    int minD = INT_MAX, maxD = 0;
    double total = 0;
    QStringList isolated;
    NetworkNode* hub = nullptr;

    for (NetworkNode* n : m_nodes) {
        int d = n->getEdgeList().size();
        total += d;
        if (d < minD) minD = d;
        if (d > maxD) { maxD = d; hub = n; }
        if (d == 0) isolated << n->label();
    }

    int V = m_nodes.size();
    double density = V > 1 ? (2.0 * m_edges.size()) / (double(V) * (V - 1)) : 0.0;

    QMap<int,int> dist;
    for (NetworkNode* n : m_nodes) dist[n->getEdgeList().size()]++;

    QStringList lines;
    lines << QString("Nodes          : %1").arg(V)
          << QString("Edges          : %1").arg(m_edges.size())
          << QString("Density        : %1").arg(density, 0, 'f', 4)
          << QString("Avg degree     : %1").arg(total / V, 0, 'f', 2)
          << QString("Min degree     : %1").arg(minD)
          << QString("Max degree     : %1  (%2)").arg(maxD).arg(hub ? hub->label() : "—")
          << QString("Isolated nodes : %1").arg(isolated.isEmpty() ? "none" : isolated.join(", "))
          << "\nDegree distribution:";
    for (auto it = dist.cbegin(); it != dist.cend(); ++it)
        lines << QString("  deg %-3d : %2 node(s)").arg(it.key()).arg(it.value());
    return lines.join("\n");
}

// ---------------------------------------------------------------
// Bipartite Check
// ---------------------------------------------------------------
QString AlgorithmPanel::algoBipartite()
{
    QMap<NetworkNode*, int> color;
    for (NetworkNode* n : m_nodes) color[n] = -1;

    bool isBipartite = true;
    QStringList conflictEdges;

    for (NetworkNode* startNode : m_nodes) {
        if (color[startNode] != -1) continue;
        QQueue<NetworkNode*> q;
        q.enqueue(startNode);
        color[startNode] = 0;
        while (!q.isEmpty() && isBipartite) {
            NetworkNode* cur = q.dequeue();
            for (NetworkEdge* e : cur->getEdgeList()) {
                NetworkNode* nb = neighbour(e, cur);
                if (color[nb] == -1) { color[nb] = 1 - color[cur]; q.enqueue(nb); }
                else if (color[nb] == color[cur]) {
                    isBipartite = false;
                    conflictEdges << QString("  %1 — %2").arg(cur->label(), nb->label());
                }
            }
        }
        if (!isBipartite) break;
    }

    if (isBipartite) {
        QStringList setA, setB;
        for (auto it = color.cbegin(); it != color.cend(); ++it)
            (it.value() == 0 ? setA : setB) << it.key()->label();
        return QString("Graph IS bipartite.\n\nSet A: { %1 }\nSet B: { %2 }")
            .arg(setA.join(", "), setB.join(", "));
    }
    return QString("Graph is NOT bipartite.\nConflicting edge(s):\n%1")
        .arg(conflictEdges.join("\n"));
}

// ---------------------------------------------------------------
// Graph Density
// ---------------------------------------------------------------
QString AlgorithmPanel::algoGraphDensity()
{
    int V = m_nodes.size(), E = m_edges.size();
    if (V == 0) return "No nodes.";

    double density  = V > 1 ? (2.0 * E) / (double(V) * (V - 1)) : 0.0;
    int    maxEdges = V > 1 ? V * (V - 1) / 2 : 0;

    double clusterSum = 0.0;
    for (NetworkNode* n : m_nodes) {
        QList<NetworkNode*> nbrs;
        for (NetworkEdge* e : n->getEdgeList()) nbrs << neighbour(e, n);
        int k = nbrs.size();
        if (k < 2) continue;
        int links = 0;
        for (int i = 0; i < nbrs.size(); ++i)
            for (int j = i + 1; j < nbrs.size(); ++j)
                for (NetworkEdge* e : nbrs[i]->getEdgeList())
                    if (neighbour(e, nbrs[i]) == nbrs[j]) { ++links; break; }
        clusterSum += (2.0 * links) / (k * (k - 1));
    }

    QString type = density < 0.2 ? "Sparse" : (density > 0.7 ? "Dense" : "Moderate");
    return QString(
        "Nodes              : %1\n"
        "Edges              : %2  /  %3 max\n"
        "Missing edges      : %4\n"
        "Density            : %5  (%6)\n"
        "Avg clustering coef: %7"
    ).arg(V).arg(E).arg(maxEdges).arg(maxEdges - E)
     .arg(density, 0, 'f', 4).arg(type)
     .arg(V > 0 ? clusterSum / V : 0.0, 0, 'f', 4);
}