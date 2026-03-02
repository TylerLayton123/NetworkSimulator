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

    if (m_w.nodeTable) {
        // selecting an item in the table sends a signal with the corresponding node pointers to select it on the graph
        connect(m_w.nodeTable, &QTableWidget::itemSelectionChanged, this, [this]() {
            if (m_syncingSelection) return;
            QList<NetworkNode*> selected;
            QSet<int> seenRows;
            for (QTableWidgetItem* item : m_w.nodeTable->selectedItems()) {
                if (seenRows.contains(item->row())) continue;
                seenRows.insert(item->row());
                auto* col0 = m_w.nodeTable->item(item->row(), 0);
                if (col0) {
                    void* ptr = col0->data(Qt::UserRole).value<void*>();
                    if (ptr) selected.append(static_cast<NetworkNode*>(ptr));
                }
            }
            m_syncingSelection = true;   
            emit tableNodesSelected(selected);
            m_syncingSelection = false;  
        });
    }

    if (m_w.edgeTable) {
        // selecting an item in the table sends a signal with the corresponding edge pointers to select it on the graph
        connect(m_w.edgeTable, &QTableWidget::itemSelectionChanged, this, [this]() {
            if (m_syncingSelection) return;
            QList<NetworkEdge*> selected;
            QSet<int> seenRows;
            for (QTableWidgetItem* item : m_w.edgeTable->selectedItems()) {
                if (seenRows.contains(item->row())) continue;
                seenRows.insert(item->row());
                auto* col0 = m_w.edgeTable->item(item->row(), 0);
                if (col0) {
                    void* ptr = col0->data(Qt::UserRole).value<void*>();
                    if (ptr) selected.append(static_cast<NetworkEdge*>(ptr));
                }
            }
            m_syncingSelection = true;
            emit tableEdgesSelected(selected);
            m_syncingSelection = false;   
        });
    }
}

// when the graph selection changes, update the tables and switch panels if needed
void GraphPanel::onGraphSelectionChanged(const QList<QGraphicsItem*>& selectedItems)
{
    if (m_syncingSelection) return;
    m_syncingSelection = true;

    QSet<void*> selectedNodePtrs, selectedEdgePtrs;
    bool hasNodes = false, hasEdges = false;
    for (QGraphicsItem* item : selectedItems) {
        if (auto* n = dynamic_cast<NetworkNode*>(item)) {
            selectedNodePtrs.insert(static_cast<void*>(n));
            hasNodes = true;
        } else if (auto* e = dynamic_cast<NetworkEdge*>(item)) {
            selectedEdgePtrs.insert(static_cast<void*>(e));
            hasEdges = true;
        }
    }

    // blockSignals prevents itemSelectionChanged from firing while we sync
    auto syncTable = [](QTableWidget* t, const QSet<void*>& ptrs) {
        if (!t) return;
        t->blockSignals(true);
        t->clearSelection();
        t->setSelectionMode(QAbstractItemView::ExtendedSelection);
        for (int row = 0; row < t->rowCount(); ++row) {
            auto* col0 = t->item(row, 0);
            if (col0 && ptrs.contains(col0->data(Qt::UserRole).value<void*>())) {
                // Use QItemSelectionModel::Select instead of selectRow() to properly accumulate rows
                t->selectionModel()->select(
                    t->model()->index(row, 0),
                    QItemSelectionModel::Select | QItemSelectionModel::Rows
                );
            }
        }
        t->blockSignals(false);
    };

    syncTable(m_w.nodeTable, selectedNodePtrs);
    syncTable(m_w.edgeTable, selectedEdgePtrs);

    if (hasEdges) showEdgeView();
    else if (hasNodes) showNodeView();

    m_syncingSelection = false;
}

// set the data and refresh the tables
void GraphPanel::setData(const QList<NetworkNode*>& nodes, const QList<NetworkEdge*>& edges) {
    m_nodes = nodes;
    m_edges = edges;
    refresh();
}

// refresh the tables and summary labels based on the current data
void GraphPanel::refresh() {
    if (m_w.nodeTable) m_w.nodeTable->blockSignals(true);
    if (m_w.edgeTable) m_w.edgeTable->blockSignals(true);

    populateNodeTable();
    populateEdgeTable();
    updateCountLabels();

    if (m_w.nodeTable) m_w.nodeTable->blockSignals(false);
    if (m_w.edgeTable) m_w.edgeTable->blockSignals(false);
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
        auto* labelItem = new QTableWidgetItem(node->label());
        labelItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(node)));
        t->setItem(row, 0, labelItem);
        

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

        auto* srcItem = new QTableWidgetItem(edge->sourceNode() ? edge->sourceNode()->label() : "?");
        srcItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(edge)));
        t->setItem(row, 0, srcItem);

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
