#include "graphpanel.h"
#include "netsim_classes.h"   // NetworkNode / NetworkEdge

#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStackedWidget>
#include <QLabel>
#include <QSplitter>
#include <QHeaderView>
#include <QFont>

// ---------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------
GraphPanel::GraphPanel(const Widgets& w, QObject* parent)
    : QObject(parent), m_w(w)
{
    // ---- splitter initial sizes: canvas 70 % / panel 30 % ----
    if (m_w.splitter) {
        m_w.splitter->setSizes({560, 240});
    }

    // ---- connect toggle buttons ----
    if (m_w.nodePanelBtn)
        connect(m_w.nodePanelBtn, &QPushButton::clicked, this, &GraphPanel::showNodeView);

    if (m_w.edgePanelBtn)
        connect(m_w.edgePanelBtn, &QPushButton::clicked, this, &GraphPanel::showEdgeView);

    // ---- make the buttons mutually exclusive ----
    if (m_w.nodePanelBtn) m_w.nodePanelBtn->setAutoExclusive(false);
    if (m_w.edgePanelBtn) m_w.edgePanelBtn->setAutoExclusive(false);

    applyStyles();

    // Show node view by default
    showNodeView();
}

// set the data and refresh the tables
void GraphPanel::setData(const QList<NetworkNode*>& nodes, const QList<NetworkEdge*>& edges) {
    m_nodes = nodes;
    m_edges = edges;
    refresh();
}

void GraphPanel::refresh() {
    populateNodeTable();
    populateEdgeTable();
    updateCountLabels();
}

// ---------------------------------------------------------------
// View switching
// ---------------------------------------------------------------
void GraphPanel::showNodeView() {
    if (m_w.panelStack) m_w.panelStack->setCurrentIndex(0);
    syncToggleButtons(true);
}

void GraphPanel::showEdgeView() {
    if (m_w.panelStack) m_w.panelStack->setCurrentIndex(1);
    syncToggleButtons(false);
}

// sync the toggle buttons to reflect which panel is active 
void GraphPanel::syncToggleButtons(bool nodesActive) {
    if (m_w.nodePanelBtn) {
        m_w.nodePanelBtn->setChecked(nodesActive);
        m_w.nodePanelBtn->setProperty("panelActive", nodesActive);
        m_w.nodePanelBtn->style()->unpolish(m_w.nodePanelBtn);
        m_w.nodePanelBtn->style()->polish(m_w.nodePanelBtn);
    }
    if (m_w.edgePanelBtn) {
        m_w.edgePanelBtn->setChecked(!nodesActive);
        m_w.edgePanelBtn->setProperty("panelActive", !nodesActive);
        m_w.edgePanelBtn->style()->unpolish(m_w.edgePanelBtn);
        m_w.edgePanelBtn->style()->polish(m_w.edgePanelBtn);
    }
}

// ---------------------------------------------------------------
// Populate node table
// ---------------------------------------------------------------
void GraphPanel::populateNodeTable() {
    QTableWidget* t = m_w.nodeTable;
    if (!t) return;

    t->setSortingEnabled(false);
    t->setRowCount(0);

    for (NetworkNode* node : m_nodes) {
        if (!node) continue;

        int row = t->rowCount();
        t->insertRow(row);

        // Label
        t->setItem(row, 0, new QTableWidgetItem(node->label()));

        // Degree — store as integer so sorting works numerically
        auto* degItem = new QTableWidgetItem();
        degItem->setData(Qt::DisplayRole, node->getEdgeList().size());
        t->setItem(row, 1, degItem);

        // Position
        QPointF p = node->pos();
        t->setItem(row, 2, new QTableWidgetItem(
            QString("(%1, %2)")
                .arg(static_cast<int>(p.x()))
                .arg(static_cast<int>(p.y()))));
    }

    t->setSortingEnabled(true);
}

// ---------------------------------------------------------------
// Populate edge table
// ---------------------------------------------------------------
void GraphPanel::populateEdgeTable() {
    QTableWidget* t = m_w.edgeTable;
    if (!t) return;

    t->setSortingEnabled(false);
    t->setRowCount(0);

    for (NetworkEdge* edge : m_edges) {
        if (!edge) continue;

        int row = t->rowCount();
        t->insertRow(row);

        t->setItem(row, 0, new QTableWidgetItem(
            edge->sourceNode() ? edge->sourceNode()->label() : "?"));

        t->setItem(row, 1, new QTableWidgetItem(
            edge->destNode() ? edge->destNode()->label() : "?"));

        t->setItem(row, 2, new QTableWidgetItem(edge->getLabel()));

        t->setItem(row, 3, new QTableWidgetItem(
            edge->isDirected() ? "Directed" : "Undirected"));
    }

    t->setSortingEnabled(true);
}

// ---------------------------------------------------------------
// Update summary labels
// ---------------------------------------------------------------
void GraphPanel::updateCountLabels() {
    if (m_w.nodeCountLbl)
        m_w.nodeCountLbl->setText(QString("Nodes: %1").arg(m_nodes.size()));

    if (m_w.edgeCountLbl)
        m_w.edgeCountLbl->setText(QString("Edges: %1").arg(m_edges.size()));
}

// ---------------------------------------------------------------
// Styles
// ---------------------------------------------------------------
void GraphPanel::applyStyles()
{
    // ---- shared table stylesheet ----
    const QString tableStyle = R"(
        QTableWidget {
            background-color: #f8f9fc;
            alternate-background-color: #edf0f8;
            gridline-color: #d0d4e0;
            font-size: 12px;
            border: none;
        }
        QTableWidget::item {
            padding: 3px 8px;
            color: #1a2a4a;
        }
        QTableWidget::item:selected {
            background-color: #3a6ab0;
            color: white;
        }
        QHeaderView::section {
            background-color: #d8dce8;
            color: #2a3a5a;
            font-weight: bold;
            font-size: 12px;
            border: none;
            border-right: 1px solid #b0b8c8;
            border-bottom: 1px solid #b0b8c8;
            padding: 4px 8px;
        }
    )";

    auto styleTable = [&](QTableWidget* t) {
        if (!t) return;
        t->setStyleSheet(tableStyle);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->verticalHeader()->setVisible(false);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setShowGrid(true);
        t->setSortingEnabled(true);
        t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        t->horizontalHeader()->setMinimumSectionSize(40);
        t->horizontalScrollBar()->setVisible(false); 
    };
    styleTable(m_w.nodeTable);
    styleTable(m_w.edgeTable);

    if (m_w.titleLbl) m_w.titleLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}
