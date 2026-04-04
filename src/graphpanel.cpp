#include "graphpanel.h"
#include "netsim_classes.h"

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
        // editing the label column updates the actual node object in the scene
        connect(m_w.nodeTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
            if (m_syncingSelection) return;
            if (item->column() != 0) return;

            int nodeId = item->data(Qt::UserRole).toInt();
            if (!m_dataHandler->nodeExists(nodeId)) return;

            m_nodeItems->value(nodeId)->setLabel(item->text());
            m_dataHandler->setNodeLabel(nodeId, item->text());
        });

        // selecting an item in the table sends a signal with the corresponding node pointers to select it on the graph
        connect(m_w.nodeTable, &QTableWidget::itemSelectionChanged, this, [this]() {
            if (m_syncingSelection) return;
            QHash<int, NetworkNode*> selected;
            QSet<int> seenRows;

            // for the selected rows
            for (QTableWidgetItem* item : m_w.nodeTable->selectedItems()) {
                if (seenRows.contains(item->row())) continue;
                seenRows.insert(item->row());
                auto* col0 = m_w.nodeTable->item(item->row(), 0);
                if (col0) {
                    int nodeId = col0->data(Qt::UserRole).toInt();
                    if (m_nodeItems->contains(nodeId))
                        selected[nodeId] = m_nodeItems->value(nodeId);
                }
            }

            m_syncingSelection = true;
            emit tableNodesSelected(selected);
            m_syncingSelection = false; 
        });
    }

    if (m_w.edgeTable) {
        // editing the label column updates the actual edge object in the scene
        connect(m_w.edgeTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
            if (m_syncingSelection) return;
            if (item->column() != 0) return;

            QPair<int,int> key = item->data(Qt::UserRole).value<QPair<int,int>>();
            if (!m_edgeItems->contains(key)) return;
            m_edgeItems->value(key)->setLabel(item->text());
            m_dataHandler->setEdgeLabel(key.first, key.second, item->text());
        });

        // selecting an item in the table sends a signal with the corresponding edge pointers to select it on the graph
        connect(m_w.edgeTable, &QTableWidget::itemSelectionChanged, this, [this]() {
            if (m_syncingSelection) return;
            QHash<QPair<int,int>, NetworkEdge*> selected;
            QSet<int> seenRows;

            for (QTableWidgetItem* item : m_w.edgeTable->selectedItems()) {
                if (seenRows.contains(item->row())) continue;
                seenRows.insert(item->row());
                auto* col0 = m_w.edgeTable->item(item->row(), 0);
                if (col0) {
                    auto key = col0->data(Qt::UserRole).value<QPair<int,int>>();
                    if (m_edgeItems->contains(key))
                        selected[key] = m_edgeItems->value(key);
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
    QSet<int> selectedNodeIds;
    QSet<QPair<int,int>> selectedEdgeKeys;

    // for each selected item in the scene
    for (QGraphicsItem* item : selectedItems) {
        // if its a node, add its pointer and id to the selected sets
        if (auto* node = dynamic_cast<NetworkNode*>(item)) {
            selectedNodeIds.insert(node->nodeId);
        } 
        // if its an edge, add its pointer and key to the selected sets
        else if (auto* edge = dynamic_cast<NetworkEdge*>(item)) {
            int srcId = edge->sourceNode()->nodeId;
            int dstId = edge->destNode()->nodeId;
            selectedEdgeKeys.insert(qMakePair(srcId, dstId));
        }
    }

    // Sync node table selection
    if (m_w.nodeTable) {
        m_w.nodeTable->blockSignals(true);
        m_w.nodeTable->clearSelection();
        for (int row = 0; row < m_w.nodeTable->rowCount(); ++row) {
            auto* col0 = m_w.nodeTable->item(row, 0);
            if (col0) {
                int nodeId = col0->data(Qt::UserRole).toInt();
                if (selectedNodeIds.contains(nodeId)) {
                    m_w.nodeTable->selectRow(row);
                }
            }
        }
        m_w.nodeTable->blockSignals(false);
        if (!selectedNodeIds.isEmpty()) {
            for (int row = 0; row < m_w.nodeTable->rowCount(); ++row) {
                auto* col0 = m_w.nodeTable->item(row, 0);
                if (col0 && selectedNodeIds.contains(col0->data(Qt::UserRole).toInt())) {
                    m_w.nodeTable->scrollTo(m_w.nodeTable->model()->index(row, 0),
                                            QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
        }
    }

    // Sync edge table selection
    if (m_w.edgeTable) {
        m_w.edgeTable->blockSignals(true);
        m_w.edgeTable->clearSelection();
        for (int row = 0; row < m_w.edgeTable->rowCount(); ++row) {
            auto* col0 = m_w.edgeTable->item(row, 0);
            if (col0) {
                auto key = col0->data(Qt::UserRole).value<QPair<int,int>>();
                if (selectedEdgeKeys.contains(key)) {
                    m_w.edgeTable->selectRow(row);
                }
            }
        }
        m_w.edgeTable->blockSignals(false);
        if (!selectedEdgeKeys.isEmpty()) {
            for (int row = 0; row < m_w.edgeTable->rowCount(); ++row) {
                auto* col0 = m_w.edgeTable->item(row, 0);
                if (col0 && selectedEdgeKeys.contains(col0->data(Qt::UserRole).value<QPair<int,int>>())) {
                    m_w.edgeTable->scrollTo(m_w.edgeTable->model()->index(row, 0),
                                            QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
        }
    }

    // Switch to the panel that has selection
    if (!selectedEdgeKeys.isEmpty())
        showEdgeView();
    else if (!selectedNodeIds.isEmpty())
        showNodeView();

    m_syncingSelection = false;
}

// set the data and refresh the tables
void GraphPanel::setData(QHash<int, NetworkNode*>* nodes, QHash<QPair<int,int>, NetworkEdge*>* edges, DataHandler* dataHandler) {
    m_nodeItems = nodes;
    m_edgeItems = edges;
    m_dataHandler = dataHandler;
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

// update the position of nodes if they are moved
void GraphPanel::updateNodePositions()
{
    QTableWidget* t = m_w.nodeTable;
    if (!t || t->rowCount() == 0) return;

    // for each row 
    for (int row = 0; row < t->rowCount(); ++row) {
        auto* col0 = t->item(row, 0);
        if (!col0) continue;

        // get the node if
        int nodeId = col0->data(Qt::UserRole).toInt();
        if (!m_nodeItems->contains(nodeId)) continue;

        // get the position of the node and update it
        QPointF p = m_nodeItems->value(nodeId)->pos();
        if (auto* posItem = t->item(row, 2))
            posItem->setText(QString("(%1, %2)")
                             .arg(static_cast<int>(p.x()))
                             .arg(static_cast<int>(p.y())));
    }
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
void GraphPanel::populateNodeTable()
{
    QTableWidget* t = m_w.nodeTable;
    if (!t) return;

    t->setSortingEnabled(false);
    t->setRowCount(0);

    const QVector<NodeInfo>* allNodes = m_dataHandler->getAllNodes();
    // loop through all node ids
    for (int nodeId = 0; nodeId < allNodes->size(); ++nodeId) {
        // skip deleted ones
        if (!m_dataHandler->nodeExists(nodeId)) continue;
        NetworkNode* node = m_nodeItems->value(nodeId, nullptr);
        if (!node) continue;

        // add the row
        int row = t->rowCount();
        t->insertRow(row);

        // Label from dataHandler
        auto* labelItem = new QTableWidgetItem(m_dataHandler->nodeLabel(nodeId));
        labelItem->setData(Qt::UserRole, nodeId);
        t->setItem(row, 0, labelItem);
        labelItem->setToolTip("Double-click to edit label");

        // Degree from dataHandler
        auto* degItem = new QTableWidgetItem();
        degItem->setData(Qt::DisplayRole, m_dataHandler->getNode(nodeId)->degree);
        degItem->setFlags(degItem->flags() & ~Qt::ItemIsEditable);
        t->setItem(row, 1, degItem);

        // Position from scene node
        QPointF p = node->pos();
        auto* posItem = new QTableWidgetItem(QString("(%1, %2)")
                                            .arg(static_cast<int>(p.x()))
                                            .arg(static_cast<int>(p.y())));
        posItem->setFlags(posItem->flags() & ~Qt::ItemIsEditable);
        t->setItem(row, 2, posItem);
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

    const QVector<NodeInfo>* allNodes = m_dataHandler->getAllNodes();
    // loop through each node
    for (int srcId = 0; srcId < allNodes->size(); ++srcId) {
        if (!m_dataHandler->nodeExists(srcId)) continue;
        // get each nodes edges
        const QVector<EdgeInfo> edges = m_dataHandler->getEdgesOf(srcId);
        for (const EdgeInfo& e : edges) {
            QPair<int,int> key = {srcId, e.destination};
            // skip edges that already exist
            NetworkEdge* edge = m_edgeItems->value(key, nullptr);
            if (!edge) continue;

            // insert the row
            int row = t->rowCount();
            t->insertRow(row);

            // Label from dataHandler
            auto* labelItem = new QTableWidgetItem(e.label);
            labelItem->setData(Qt::UserRole, QVariant::fromValue(key));
            t->setItem(row, 0, labelItem);

            // Source node label from dataHandler
            auto* srcItem = new QTableWidgetItem(m_dataHandler->nodeLabel(srcId));
            srcItem->setFlags(srcItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(row, 1, srcItem);

            // Destination node label from dataHandler
            auto* dstItem = new QTableWidgetItem(m_dataHandler->nodeLabel(e.destination));
            dstItem->setFlags(dstItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(row, 2, dstItem);
        }
    }

    t->setSortingEnabled(true);
}

// ---------------------------------------------------------------
// Update summary labels
// ---------------------------------------------------------------
void GraphPanel::updateCountLabels() {
    if (m_w.nodeCountLbl)
        m_w.nodeCountLbl->setText(QString("Nodes: %1").arg(m_dataHandler->nodeCount()));

    if (m_w.edgeCountLbl)
        m_w.edgeCountLbl->setText(QString("Edges: %1").arg(m_dataHandler->edgeCount()));
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
        QTableWidget::item:focus {
            outline: none;
            border: none;
            background-color: transparent;
        }
        QTableWidget::item:selected:focus {
            background-color: #3a6ab0;
            outline: none;
            border: none;
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
        t->setEditTriggers(QAbstractItemView::DoubleClicked); 
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