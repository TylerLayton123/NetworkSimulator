#include "algorithmpanel.h"
#include "netsim_classes.h"

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
            ? QString("  Source: %1").arg(src->label())
            : "  Source: none");
    }
}

void AlgorithmPanel::setSourceNode(NetworkNode* node)
{
    m_sourceNode = node;
    if (m_sourceInfo && node)
        m_sourceInfo->setText(
            QString("  Source: %1").arg(node->label()));
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

    // toggle buttons for page switching
    m_searchBtn  = new QPushButton("Search");
    m_visualsBtn = new QPushButton("Visualization");
    m_searchBtn->setCheckable(true);
    m_visualsBtn->setCheckable(true);
    m_searchBtn->setAutoExclusive(true);
    m_visualsBtn->setAutoExclusive(true);
    titleLayout->addWidget(m_searchBtn);
    titleLayout->addWidget(m_visualsBtn);

    connect(m_searchBtn,  &QPushButton::clicked, this, &AlgorithmPanel::showSearchPage);
    connect(m_visualsBtn, &QPushButton::clicked, this, &AlgorithmPanel::showVisualPage);

    // Column header 
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

    // Stacked pages
    m_stack = new QStackedWidget;

    // algorithms lists with descriptions and "Run" buttons
    const QList<QPair<QString,QString>> searchAlgos = {
        { "bfs", "Breadth-first traversal from a source node" },
        { "dfs", "Depth-first traversal from a source node" },
        { "dijkstra", "Shortest paths — uses edge labels as weights" },
        // { "cycle", "Detect cycles via union-find" },
        { "components", "Count and list all connected components" },
        // { "topo", "Topological sort (DAGs only)" },
    };

    const QList<QPair<QString,QString>> visualAlgos = {
        // { "mst", "Minimum spanning tree (Kruskal's)" },
        // { "degree", "Degree distribution and graph metrics" },
        // { "bipartite", "Check if the graph is bipartite (2-colorable)" },
        // { "density", "Edge density, clustering coefficient, summary" },
        { "sfdp", "Scalable force-directed placement layout" },
        { "circular", "Arrange nodes evenly around a circle" },
        { "spiral", "Arrange nodes along a spiral" },
    };

    m_stack->addWidget(buildAlgoPage(searchAlgos));   
    m_stack->addWidget(buildAlgoPage(visualAlgos)); 

    // Source info bar 
    m_sourceInfo = new QLabel("  Source: none");
    m_sourceInfo->setFixedHeight(18);
    m_sourceInfo->setStyleSheet(
        "background-color: #e8ecf5; color: #3a4a6a; font-size: 10px;"
        "border-top: 1px solid #b0b8c8; border-bottom: 1px solid #b0b8c8;"
    );

    // Output area 
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

    // SFDP stop button (hidden until SFDP is running)
    m_sfdpStopBtn = new QPushButton("Stop Layout");
    m_sfdpStopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #b03a3a; color: white;"
        "  border: none; padding: 4px 12px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #c04a4a; }"
    );
    m_sfdpStopBtn->hide();
    connect(m_sfdpStopBtn, &QPushButton::clicked, this, &AlgorithmPanel::stopSFDP);

    root->addWidget(titleBar);
    root->addWidget(colHeader);
    root->addWidget(m_stack, 1);
    root->addWidget(m_sourceInfo);
    root->addWidget(m_sfdpStopBtn);
    root->addWidget(m_output);

    showSearchPage();
}

// builds the scrollable list of algorithms with descriptions and "Run" buttons
QWidget* AlgorithmPanel::buildAlgoPage(const QList<QPair<QString,QString>>& algos)
{
    static const QMap<QString,QString> names = {
        { "bfs", "BFS"},
        { "dfs", "DFS"},
        { "dijkstra", "Dijkstra"},
        // { "cycle", "Cycle Detect"},
        { "components", "Components"},
        // { "topo", "Topo Sort"},
        // { "mst", "MST (Kruskal)"},
        // { "degree", "Degree Stats"},
        // { "bipartite", "Bipartite"},
        // { "density", "Density"},
        { "sfdp", "SFDP Layout"},
        { "circular", "Circular Layout"},
        { "spiral", "Spiral Layout"},
    };

    // scrollable area
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget;
    auto* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    bool alt = false;
    // add each algorithm as a row with name, description, and "Run" button
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

        // name label
        auto* nameLbl = new QLabel(names.value(id, id));
        nameLbl->setFixedWidth(130);
        QFont nf = nameLbl->font(); nf.setBold(true); nf.setPointSize(10);
        nameLbl->setFont(nf);
        nameLbl->setStyleSheet("color: #1a2a4a;");

        // description label
        auto* descLbl = new QLabel(desc);
        descLbl->setStyleSheet("color: #4a5a7a; font-size: 10px;");
        descLbl->setWordWrap(false);

        // run button to run the algorihtms
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

        // connect the button to run the algorithm with the given id when clicked
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
// Page switching
// ---------------------------------------------------------------
void AlgorithmPanel::showSearchPage()
{
    if (m_stack) m_stack->setCurrentIndex(0);
    if (m_searchBtn)  m_searchBtn->setChecked(true);
    if (m_visualsBtn) m_visualsBtn->setChecked(false);
}

void AlgorithmPanel::showVisualPage()
{
    if (m_stack) m_stack->setCurrentIndex(1);
    if (m_visualsBtn) m_visualsBtn->setChecked(true);
    if (m_searchBtn)  m_searchBtn->setChecked(false);
}

// ---------------------------------------------------------------
// Node-picker dialog  (BFS / DFS / Dijkstra)
// ---------------------------------------------------------------
bool AlgorithmPanel::askParams(const QString& algoName, bool needsSource, bool needsTarget, AlgoParams& out)
{
    if (m_nodes.isEmpty()) return false;

    QDialog dlg(this);
    dlg.setWindowTitle(algoName);
    dlg.setMinimumWidth(300);

    auto* form  = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);

    // ask for the source node (if needed)
    QComboBox* sourceCbo = nullptr;
    if (needsSource) {
        sourceCbo = new QComboBox;
        for (NetworkNode* n : m_nodes)
            sourceCbo->addItem(n->label(), QVariant::fromValue(static_cast<void*>(n)));
        NetworkNode* presel = sourceOrFirst();
        if (presel) {
            int idx = m_nodes.indexOf(presel);
            if (idx >= 0) sourceCbo->setCurrentIndex(idx);
        }
        form->addRow("Source node:", sourceCbo);
    }

    // ask for the target node (if needed)
    QComboBox* targetCbo = nullptr;
    if (needsTarget) {
        targetCbo = new QComboBox;
        targetCbo->addItem("(none — show all)", QVariant::fromValue(static_cast<void*>(nullptr)));
        for (NetworkNode* n : m_nodes)
            targetCbo->addItem(n->label(), QVariant::fromValue(static_cast<void*>(n)));
        form->addRow("Target node (optional):", targetCbo);
    }

    // OK / Cancel buttons
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    if (sourceCbo)
        out.source = static_cast<NetworkNode*>(sourceCbo->currentData().value<void*>());
    if (targetCbo)
        out.target = static_cast<NetworkNode*>(targetCbo->currentData().value<void*>());

    return true;
}


// ---------------------------------------------------------------
// SFDP parameter dialog
// ---------------------------------------------------------------
bool AlgorithmPanel::askSFDPParams(SFDPParams& out) {
    // saved parameters
    out = m_sfdpParams;

    QDialog dlg(this);
    dlg.setWindowTitle("SFDP Layout Parameters");
    dlg.setMinimumWidth(320);

    auto* form   = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);

    // Header description
    auto* descLbl = new QLabel(
        "Positions nodes using a force-directed model.\n"
        "Attractive forces pull connected nodes together;\n"
        "repulsive forces push all nodes apart.");
    descLbl->setWordWrap(true);
    descLbl->setStyleSheet("color: #4a5a7a; font-size: 10px; padding-bottom: 6px;");
    layout->addWidget(descLbl);
    layout->addLayout(form);

    // Iterations
    auto* iterSpin = new QSpinBox;
    iterSpin->setRange(1, 2000);
    iterSpin->setValue(out.iterations);
    iterSpin->setSuffix(" steps");
    form->addRow("Iterations:", iterSpin);

    // K — ideal edge length
    auto* kSpin = new QDoubleSpinBox;
    kSpin->setRange(10.0, 2000.0);
    kSpin->setValue(out.K);
    kSpin->setSingleStep(10.0);
    kSpin->setSuffix(" px");
    kSpin->setToolTip("Ideal distance between connected nodes (scene pixels).");
    form->addRow("Ideal edge length (K):", kSpin);

    // C — repulsion constant
    auto* cSpin = new QDoubleSpinBox;
    cSpin->setRange(0.01, 10.0);
    cSpin->setValue(out.C);
    cSpin->setSingleStep(0.05);
    cSpin->setDecimals(3);
    cSpin->setToolTip("Repulsion strength. Higher = nodes spread further apart.");
    form->addRow("Repulsion constant (C):", cSpin);

    // Tolerance
    auto* tolSpin = new QDoubleSpinBox;
    tolSpin->setRange(0.001, 100.0);
    tolSpin->setValue(out.tol);
    tolSpin->setSingleStep(0.1);
    tolSpin->setDecimals(3);
    tolSpin->setToolTip("Stop early when all nodes move less than K × tol per step.");
    form->addRow("Convergence tolerance:", tolSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    // set the values chosen
    out.iterations = iterSpin->value();
    out.K = kSpin->value();
    out.C = cSpin->value();
    out.tol = tolSpin->value();

    // save for next time
    m_sfdpParams = out;
    return true;
}

// format the elapsed time
QString formatTimer(QElapsedTimer& timer) {
    // stop timer
    qint64 elapsedUs = timer.nsecsElapsed() / 1000;
    QString timeStr = elapsedUs < 1000
        ? QString("%1 µs").arg(elapsedUs)
        : elapsedUs < 1000000
            ? QString("%1 ms").arg(elapsedUs / 1000.0, 0, 'f', 2)
            : QString("%1 s").arg(elapsedUs / 1000000.0, 0, 'f', 3);

    return QString("\nTime: %1").arg(timeStr);
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

    // Stop any running SFDP before starting something new
    if (m_sfdpTimer && m_sfdpTimer->isActive())
        stopSFDP();

    QString title, result;
    AlgoParams p;

    // dispatch based on the algorithm ID; ask for parameters if needed, then run and print results
    if (id == "bfs" || id == "dfs" || id == "dijkstra") {
        if (!askParams(id.toUpper(), true, true, p)) return;
        if (id == "bfs") {
            title = "BFS";      
            result = algoBFS(p.source, p.target);
        }
        else if (id == "dfs") {
            title = "DFS";
            result = algoDFS(p.source, p.target); 
        }
        else if (id == "dijkstra") {
            title = "Dijkstra"; 
            result = algoDijkstra(p.source, p.target); 
        }
    }
    else if (id == "sfdp") {
        SFDPParams sp;
        if (!askSFDPParams(sp)) return;
        runSFDP(sp);
    }
    else if (id == "circular") {
        title = "Circular Layout";
        result = algoCircularLayout();
    }
    else if (id == "spiral") {
        title = "Spiral Layout";
        result = algoSpiralLayout();
    }
    // else if (id == "cycle")      { title = "Cycle Detection";      result = algoCycleDetection(); }
    else if (id == "components") { title = "Connected Components"; result = algoConnectedComponents(); }
    // else if (id == "topo")       { title = "Topological Sort";     result = algoTopoSort(); }
    // else if (id == "mst")        { title = "MST (Kruskal's)";      result = algoMST(); }
    // else if (id == "degree")     { title = "Degree Statistics";    result = algoDegreeStats(); }
    // else if (id == "bipartite")  { title = "Bipartite Check";      result = algoBipartite(); }
    // else if (id == "density")    { title = "Graph Density";        result = algoGraphDensity(); }

    printResult(title, result);
}

// print the result of an algorithm in the output area
void AlgorithmPanel::printResult(const QString& title, const QString& body)
{
    if (m_output)
        m_output->setPlainText(QString("=== %1 ===\n%2").arg(title, body));
}

// ---------------------------------------------------------------
// Visulaization Algorithms
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// SFDP — initialise and start animation timer
// ---------------------------------------------------------------
void AlgorithmPanel::runSFDP(const SFDPParams& p)
{
    // size check
    int N = m_nodes.size();
    if (N < 2) {
        printResult("SFDP Layout", "Need at least 2 nodes.");
        return;
    }

    // Store parameters
    m_sfdpMaxIter  = p.iterations;
    m_sfdpK        = p.K;
    m_sfdpC        = p.C;
    m_sfdpTol      = p.tol;
    m_sfdpIter     = 0;
    m_sfdpProgress = 0;
    m_sfdpStopFlag = false;
    m_sfdpN        = N;

    // Initial step size — half the ideal edge length gives good initial movement
    m_sfdpStep   = p.K * 0.5;
    m_sfdpEnergy = std::numeric_limits<double>::max();

    // Snapshot current scene positions as starting positions
    m_sfdpPos.resize(N);
    for (int i = 0; i < N; ++i)
        m_sfdpPos[i] = m_nodes[i]->pos();

    // Build flat N×N adjacency matrix (undirected)
    m_sfdpAdj.fill(false, N * N);
    for (NetworkEdge* e : m_edges) {
        int si = m_nodes.indexOf(e->sourceNode());
        int di = m_nodes.indexOf(e->destNode());
        if (si >= 0 && di >= 0) {
            m_sfdpAdj[si * N + di] = true;
            m_sfdpAdj[di * N + si] = true;
        }
    }

    // Show stop button
    if (m_sfdpStopBtn) m_sfdpStopBtn->show();

    // Create / start the timer (~60 fps)
    if (!m_sfdpTimer) {
        m_sfdpTimer = new QTimer(this);
        connect(m_sfdpTimer, &QTimer::timeout, this, &AlgorithmPanel::sfdpStep);
    }
    m_sfdpTimer->start(16);

    printResult("SFDP Layout",
        QString("Running…  iteration 0 / %1").arg(m_sfdpMaxIter));
}

// ---------------------------------------------------------------
// SFDP — single iteration (called by timer)
// ---------------------------------------------------------------
void AlgorithmPanel::sfdpStep()
{
    // Guard: stop if nodes list changed (e.g. user deleted a node)
    if (m_nodes.size() != m_sfdpN) {
        stopSFDP();
        printResult("SFDP Layout", "Stopped — graph was modified during layout.");
        return;
    }

    if (m_sfdpIter >= m_sfdpMaxIter || m_sfdpStopFlag) {
        stopSFDP();
        return;
    }

    int    N    = m_sfdpN;
    double K    = m_sfdpK;
    double C    = m_sfdpC;
    double step = m_sfdpStep;

    QVector<QPointF> newPos = m_sfdpPos;
    double energy = 0.0;

    // Compute forces and new positions for each node
    for (int i = 0; i < N; ++i) {
        double fx = 0.0, fy = 0.0;

        for (int j = 0; j < N; ++j) {
            if (i == j) continue;

            // Compute distance and unit vector from i to j
            double dx   = m_sfdpPos[j].x() - m_sfdpPos[i].x();
            double dy   = m_sfdpPos[j].y() - m_sfdpPos[i].y();
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1e-6) {
                // Two nodes at the exact same position: apply a small random nudge
                double angle = (double)std::rand() / RAND_MAX * 2.0 * M_PI;
                fx += std::cos(angle) * K * 0.1;
                fy += std::sin(angle) * K * 0.1;
                continue;
            }

            double ux = dx / dist; 
            double uy = dy / dist;

            double mag;
            // connected nodes attract while non-connected nodes repel
            if (m_sfdpAdj[i * N + j]) {
                // Attractive force: f_attr = dist^2 / K  (pulls i towards j)
                mag = (dist * dist) / K;
            } else {
                // Repulsive force: f_repln = -C * K^2 / dist  (pushes i away from j)
                mag = -C * (K * K) / dist;
            }

            fx += mag * ux;
            fy += mag * uy;
        }

        // Normalise force and move by step in that direction
        double fmag = std::sqrt(fx * fx + fy * fy);
        if (fmag < 1e-10) {
            // Zero net force — no movement needed
            continue;
        }

        // new positions 
        newPos[i].setX(m_sfdpPos[i].x() + step * (fx / fmag));
        newPos[i].setY(m_sfdpPos[i].y() + step * (fy / fmag));
        energy += fmag * fmag; 
    }

    // ── Adaptive step / cooling schedule ──────────────────────────
    // If energy is decreasing, reward with occasional step increase (heat)
    // otherwise cool down.
    const double t = 0.9;
    if (energy < m_sfdpEnergy) {
        m_sfdpProgress++;
        if (m_sfdpProgress >= 5) {
            m_sfdpProgress = 0;
            m_sfdpStep /= t; 
        }
    } else {
        m_sfdpProgress = 0;
        m_sfdpStep *= t; 
    }
    m_sfdpEnergy = energy;

    // Convergence check (dist_tolerance) 
    // Stop when every node moves less than K × tol this iteration
    bool converged = true;
    for (int i = 0; i < N; ++i) {
        double dx = newPos[i].x() - m_sfdpPos[i].x();
        double dy = newPos[i].y() - m_sfdpPos[i].y();
        if (std::sqrt(dx * dx + dy * dy) >= K * m_sfdpTol) {
            converged = false;
            break;
        }
    }

    // Apply new positions to scene nodes
    m_sfdpPos = newPos;
    for (int i = 0; i < N; ++i)
        m_nodes[i]->setPos(m_sfdpPos[i]);

    m_sfdpIter++;

    // Update output area with live progress
    m_output->setPlainText(
        QString("=== SFDP Layout ===\n"
                "Iteration : %1 / %2\n"
                "Energy    : %3\n"
                "Step size : %4")
            .arg(m_sfdpIter).arg(m_sfdpMaxIter)
            .arg(energy,       0, 'f', 2)
            .arg(m_sfdpStep,   0, 'f', 4));

    if (converged) {
        m_sfdpStopFlag = true;
        stopSFDP();
        m_output->setPlainText(
            QString("=== SFDP Layout ===\n"
                    "Converged after %1 iteration(s).\n"
                    "Final energy : %2")
                .arg(m_sfdpIter)
                .arg(energy, 0, 'f', 2));
    }
}

// ---------------------------------------------------------------
// SFDP — stop
// ---------------------------------------------------------------
void AlgorithmPanel::stopSFDP()
{
    if (m_sfdpTimer) m_sfdpTimer->stop();
    if (m_sfdpStopBtn) m_sfdpStopBtn->hide();

    if (!m_sfdpStopFlag && m_sfdpIter > 0) {
        // User pressed Stop or max iterations reached — append final note
        QString current = m_output->toPlainText();
        if (!current.contains("Stopped") && !current.contains("Converged")) {
            m_output->setPlainText(
                QString("=== SFDP Layout ===\n"
                        "Stopped at iteration %1 / %2.\n"
                        "Final energy : %3")
                    .arg(m_sfdpIter).arg(m_sfdpMaxIter)
                    .arg(m_sfdpEnergy, 0, 'f', 2));
        }
    }
}

// ---------------------------------------------------------------
// Circular Layout
// ---------------------------------------------------------------
void AlgorithmPanel::runCircularLayout(bool askUser) {
    printResult("Circular Layout", algoCircularLayout(askUser));
}

// ask the user for spacing parameter
bool AlgorithmPanel::askCircularParams(CircularParams& out) {
    // load saved parameters
    out = m_circularParams;

    QDialog dlg(this);
    dlg.setWindowTitle("Circular Layout Parameters");
    dlg.setMinimumWidth(300);

    auto* form   = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);

    // description
    auto* descLbl = new QLabel(
        "Arranges all nodes evenly around a circle.\n"
        "Spacing controls how far apart adjacent nodes sit.\n"
        "Larger values produce a bigger circle.");
    descLbl->setWordWrap(true);
    descLbl->setStyleSheet("color: #4a5a7a; font-size: 10px; padding-bottom: 6px;");
    layout->addWidget(descLbl);
    layout->addLayout(form);

    // Live radius preview label
    auto* previewLbl = new QLabel;
    previewLbl->setStyleSheet("color: #3a6ab0; font-size: 10px;");

    // Spacing input
    auto* spacingSpin = new QDoubleSpinBox;
    spacingSpin->setRange(5.0, 500.0);
    spacingSpin->setValue(out.spacing);
    spacingSpin->setSingleStep(5.0);
    spacingSpin->setSuffix(" px / node");
    spacingSpin->setToolTip("Distance per node added to the minimum radius of 250 px.");
    form->addRow("Node spacing:", spacingSpin);
    form->addRow("Radius preview:", previewLbl);

    // Update preview whenever spacing changes
    auto updatePreview = [&]() {
        int    N         = m_nodes.size();
        double minRadius = (N * 60.0) / (2.0 * M_PI);
        double radius    = minRadius + N * spacingSpin->value();
        previewLbl->setText(QString("%1 px  (min: %2 px,  %3 nodes)")
                                .arg(qRound(radius))
                                .arg(qRound(minRadius))
                                .arg(N));
    };
    updatePreview();
    connect(spacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [&](double) { updatePreview(); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    out.spacing = spacingSpin->value();
    m_circularParams = out;
    return true;
}

QString AlgorithmPanel::algoCircularLayout(bool askUser)
{
    // size check
    int N = m_nodes.size();
    if (N == 0) return "No nodes to arrange.";
    if (N == 1) {
        m_nodes[0]->setPos(0, 0);
        return "Only one node — placed at origin.";
    }

    // get the parameter
    CircularParams cp;
    if (askUser) {
        if (!askCircularParams(cp)) return "Cancelled.";
    } else {
        cp = m_circularParams;
    }

    // min radius is 50
    const qreal slotSize = 60.0;
    qreal minRadius = (N * slotSize) / (2.0 * M_PI);
    qreal radius = minRadius + N * cp.spacing;

    // start timer
    QElapsedTimer timer;
    timer.start();

    // arrange nodes evenly around the circle
    for (int i = 0; i < N; ++i) {
        qreal angle = 2.0 * M_PI * i / N;
        m_nodes[i]->setPos(radius * std::cos(angle), radius * std::sin(angle));
    }

    // rearrange the first node
    qreal angle = 2.0 * M_PI * 0 / N;
    m_nodes[0]->setPos(radius * std::cos(angle), radius * std::sin(angle));

    return QString("Arranged %1 node(s) on a circle.\nRadius : %2 px\nSpacing: %3 px / node\n%4")
               .arg(N).arg(qRound(radius)).arg(cp.spacing, 0, 'f', 1).arg(formatTimer(timer));
}

// ---------------------------------------------------------------
// Spiral Layout
// ---------------------------------------------------------------
void AlgorithmPanel::runSpiralLayout(bool askUser) {
    printResult("Spiral Layout", algoSpiralLayout(askUser));
}

// pop up for the spiral parameters
bool AlgorithmPanel::askSpiralParams(SpiralParams& out) {
    out = m_spiralParams;

    QDialog dlg(this);
    dlg.setWindowTitle("Spiral Layout Parameters");
    dlg.setMinimumWidth(300);

    auto* form   = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);

    auto* descLbl = new QLabel(
        "Arranges nodes along a spiral.\n"
        "Spacing controls arc-length distance between adjacent nodes.\n"
        "Radius growth controls how quickly the spiral expands outward.");
    descLbl->setWordWrap(true);
    descLbl->setStyleSheet("color: #4a5a7a; font-size: 10px; padding-bottom: 6px;");
    layout->addWidget(descLbl);
    layout->addLayout(form);

    auto* previewLbl = new QLabel;
    previewLbl->setStyleSheet("color: #3a6ab0; font-size: 10px;");

    // get arc length
    auto* spacingSpin = new QDoubleSpinBox;
    spacingSpin->setRange(20.0, 500.0);
    spacingSpin->setValue(out.spacing);
    spacingSpin->setSingleStep(5.0);
    spacingSpin->setSuffix(" px");
    spacingSpin->setToolTip("Arc-length distance between consecutive nodes along the spiral.");
    form->addRow("Node spacing:", spacingSpin);

    // get radius growth
    auto* radiusGrowthSpin = new QDoubleSpinBox;
    radiusGrowthSpin->setRange(10.0, 500.0);
    radiusGrowthSpin->setValue(out.radiusGrowth);
    radiusGrowthSpin->setSingleStep(5.0);
    radiusGrowthSpin->setSuffix(" px / rad");
    radiusGrowthSpin->setToolTip("How quickly the spiral arm expands per radian. Larger = looser coils.");
    form->addRow("Radius growth:", radiusGrowthSpin);

    form->addRow("Outer radius preview:", previewLbl);

    // Preview: estimate the outer radius by running the theta accumulation
    auto updatePreview = [&]() {
        int N = m_nodes.size();
        if (N == 0) { previewLbl->setText("No nodes"); return; }

        qreal spacing = spacingSpin->value();
        qreal growth  = radiusGrowthSpin->value();
        qreal theta   = 1.0;
        for (int i = 1; i < N; ++i)
            theta += spacing / (growth * theta);

        previewLbl->setText(QString("%1 px  (%2 nodes)")
                                .arg(qRound(growth * theta))
                                .arg(N));
    };
    updatePreview();
    connect(spacingSpin,      QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [&](double) { updatePreview(); });
    connect(radiusGrowthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [&](double) { updatePreview(); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    out.spacing     = spacingSpin->value();
    out.radiusGrowth = radiusGrowthSpin->value();
    m_spiralParams  = out;
    return true;
}

// spiral layout algorithm
QString AlgorithmPanel::algoSpiralLayout(bool askUser)
{
    int N = m_nodes.size();
    if (N == 0) return "No nodes to arrange.";
    if (N == 1) {
        m_nodes[0]->setPos(0, 0);
        return "Only one node — placed at origin.";
    }

    SpiralParams sp;
    if (askUser) {
        if (!askSpiralParams(sp)) return "Cancelled.";
    } else {
        sp = m_spiralParams;
    }

    const qreal spacing     = sp.spacing;
    const qreal radiusGrowth = sp.radiusGrowth;

    // start timer
    QElapsedTimer timer;
    timer.start();

    // theta accumulation
    QVector<qreal> thetas(N);
    thetas[0] = 1.0;
    for (int i = 1; i < N; ++i) {
        qreal r = radiusGrowth * thetas[i - 1];
        thetas[i] = thetas[i - 1] + spacing / r;
    }

    qreal cosT = 1.0;
    qreal sinT = 0.0;
    qreal prevTheta = thetas[0];

    m_nodes[0]->setPos(radiusGrowth * thetas[0], 0.0);

    // for each node, arrange around in a spiral
    for (int i = 1; i < N; ++i) {
        qreal dTheta = thetas[i] - prevTheta;
        prevTheta = thetas[i];

        qreal cosDT  = qCos(dTheta);
        qreal sinDT  = qSin(dTheta);
        qreal newCos = cosT * cosDT - sinT * sinDT;
        qreal newSin = sinT * cosDT + cosT * sinDT;
        cosT = newCos;
        sinT = newSin;

        qreal r = radiusGrowth * thetas[i];
        m_nodes[i]->setPos(r * cosT, r * sinT);
    }

    qreal outerRadius = radiusGrowth * thetas[N - 1];
    return QString("Arranged %1 node(s) on a spiral.\nOuter radius: %2 px\nSpacing: %3 px\nRadius growth: %4 px/rad\n%5")
               .arg(N)
               .arg(qRound(outerRadius))
               .arg(spacing,      0, 'f', 1)
               .arg(radiusGrowth, 0, 'f', 1)
               .arg(formatTimer(timer));
}


// ---------------------------------------------------------------
// Search Algorithms
// ---------------------------------------------------------------

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
// BFS
// ---------------------------------------------------------------
QString AlgorithmPanel::algoBFS(NetworkNode* source, NetworkNode* target)
{
    if (!source) return "No source node.";

    QMap<NetworkNode*, bool> visited;
    QMap<NetworkNode*, NetworkNode*> prev;
    QQueue<NetworkNode*> queue;
    QStringList order;
    bool foundTarget = false;

    visited[source] = true;
    prev[source] = nullptr;
    queue.enqueue(source);

    // start timer
    QElapsedTimer timer;
    timer.start();

    // standard BFS loop
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

    // format results
    QStringList lines;
    lines << QString("Source     : %1").arg(source->label());
    if (target) {
        lines << QString("Target     : %1").arg(target->label());
        if (foundTarget) {
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Found after: %1 step(s)").arg(path.size() - 1);
            lines << QString("Path       : %1").arg(path.join(" -> "));
        } else {
            lines << "Target not reachable from source.";
        }
        lines << "";
    }
    lines << QString("Visit order: %1").arg(order.join(" -> "));
    lines << QString("Visited    : %1 / %2").arg(visited.size()).arg(m_nodes.size());
    int unreached = m_nodes.size() - visited.size();
    if (unreached > 0)
        lines << QString("Unreached  : %1 node(s) (disconnected)").arg(unreached);

    lines << formatTimer(timer);
    return lines.join("\n");
}

// ---------------------------------------------------------------
// DFS
// ---------------------------------------------------------------
QString AlgorithmPanel::algoDFS(NetworkNode* source, NetworkNode* target)
{
    if (!source) return "No source node.";

    QSet<NetworkNode*>               visited;
    QMap<NetworkNode*, NetworkNode*> prev;
    QStack<NetworkNode*>             stack;
    QStringList order;
    bool foundTarget = false;

    stack.push(source);
    prev[source] = nullptr;

    // start timer
    QElapsedTimer timer;
    timer.start();

    // standard DFS loop
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

    // format results
    QStringList lines;
    lines << QString("Source     : %1").arg(source->label());
    if (target) {
        lines << QString("Target     : %1").arg(target->label());
        if (foundTarget) {
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Found after: %1 step(s)").arg(path.size() - 1);
            lines << QString("Path       : %1").arg(path.join(" -> "));
        } else {
            lines << "Target not reachable from source.";
        }
        lines << "";
    }
    lines << QString("Visit order: %1").arg(order.join(" -> "));
    lines << QString("Visited    : %1 / %2").arg(visited.size()).arg(m_nodes.size());
    int unreached = m_nodes.size() - visited.size();
    if (unreached > 0)
        lines << QString("Unreached  : %1 node(s) (disconnected)").arg(unreached);
    lines << formatTimer(timer);
    return lines.join("\n");
}

// ---------------------------------------------------------------
// Dijkstra
// ---------------------------------------------------------------
QString AlgorithmPanel::algoDijkstra(NetworkNode* source, NetworkNode* target)
{
    if (!source) return "No source node.";
    if (m_nodes.size() < 2) return "Need at least 2 nodes.";

    // start timer
    QElapsedTimer timer;
    timer.start();

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

    // standard Dijkstra's loop
    while (!unvisited.isEmpty()) {
        NetworkNode* u = nullptr;
        for (NetworkNode* n : unvisited) if (!u || dist[n] < dist[u]) u = n;
        if (dist[u] == INF) break;
        if (target && u == target) break;
        unvisited.removeOne(u);
        for (NetworkEdge* e : u->getEdgeList()) {
            NetworkNode* v = neighbour(e, u);
            if (!unvisited.contains(v)) continue;
            double alt = dist[u] + edgeWeight(e);
            if (alt < dist[v]) { dist[v] = alt; prev[v] = u; }
        }
    }

    // format results
    QStringList lines;
    lines << QString("Source: %1%2").arg(source->label())
             .arg(allNumeric ? "" : "  [non-numeric labels treated as weight 1]");

    if (target) {
        lines << QString("Target: %1").arg(target->label()) << "";
        if (dist[target] == INF) {
            lines << "Target is unreachable from source.";
        } else {
            QStringList path;
            for (NetworkNode* cur = target; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("Distance : %1").arg(QString::number(dist[target], 'f', 2));
            lines << QString("Path     : %1").arg(path.join(" -> "));
        }
        lines << formatTimer(timer);
        return lines.join("\n");
    }

    lines << "" << "Node        Distance   Path" << QString(45, '-');
    for (NetworkNode* n : m_nodes) {
        if (n == source) continue;
        if (dist[n] == INF) {
            lines << QString("%-12s unreachable").arg(n->label());
        } else {
            QStringList path;
            for (NetworkNode* cur = n; cur; cur = prev.value(cur, nullptr))
                path.prepend(cur->label());
            lines << QString("%1 %2 %3")
                .arg(n->label(),                          -12)
                .arg(QString::number(dist[n], 'f', 2),    -10)
                .arg(path.join(" -> "));
        }
    }
    lines << formatTimer(timer);
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

    // start timer
    QElapsedTimer timer;
    timer.start();

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
    lines << formatTimer(timer);
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
        .arg(order.join(" -> ")).arg(order.size());
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
    lines << QString("%1 %2").arg("Edge", -22).arg("Weight") << QString(32, '-');
    for (NetworkEdge* e : mst) {
        QString pair = QString("%1 — %2").arg(e->sourceNode()->label(), e->destNode()->label());
        lines << QString("%1 %2").arg(pair, -22).arg(e->getLabel().isEmpty() ? "1" : e->getLabel());
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