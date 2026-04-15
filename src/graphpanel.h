#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QPair>
#include "datahandler.h"
#include "netsim_classes.h"

class NetworkNode;
class NetworkEdge;
class QGraphicsItem;
class NetSim;
class QPushButton;
class QTableWidget;
class QStackedWidget;
class QLabel;
class QSplitter;

// ---------------------------------------------------------------
// GraphPanel
// ---------------------------------------------------------------
class GraphPanel : public QObject
{
    Q_OBJECT

public:
    struct Widgets {
        QPushButton*    nodePanelBtn = nullptr;
        QPushButton*    edgePanelBtn = nullptr;
        QTableWidget*   nodeTable    = nullptr;
        QTableWidget*   edgeTable    = nullptr;
        QStackedWidget* panelStack   = nullptr;
        QLabel*         nodeCountLbl = nullptr;
        QLabel*         edgeCountLbl = nullptr;
        QLabel*         nodeItemsLbl = nullptr;
        QLabel*         edgeItemsLbl = nullptr;
        QLabel*         titleLbl     = nullptr;
        QSplitter*      splitter     = nullptr; 
    };

    explicit GraphPanel(NetSim* netSim, const Widgets& w, QObject* parent = nullptr);

    void setData(QHash<int, NetworkNode*>* nodes, QHash<QPair<int,int>, NetworkEdge*>* edges, DataHandler* dataHandler);
    void refresh();
    void clear();
    void addNodeRow(int nodeId);
    void removeNodeRow(int nodeId);
    void addEdgeRow(int srcId, int dstId);
    void removeEdgeRow(int srcId, int dstId);
    void onGraphSelectionChanged(const QList<QGraphicsItem*>& selectedItems);
    // void updateNodePositions();
    void rebuildNodeRowIndex();
    void rebuildEdgeRowIndex();

    // targeted single-row updates 
    void updateNodeRow(int nodeId); 
    void updateEdgeRow(int srcId, int dstId);

signals:
    void tableNodesSelected(QHash<int, NetworkNode*>& nodes);
    void tableEdgesSelected(QHash<QPair<int,int>, NetworkEdge*>& edges);
    void contractRequested();
    void deleteRequested();
    void findRequested();
    void expandRequested(int nodeId);

private slots:
    void showNodeView();
    void showEdgeView();

private:
    void applyStyles();
    void populateNodeTable();
    void populateEdgeTable();
    void updateCountLabels();
    void syncToggleButtons(bool nodesActive);

    void showNodeContextMenu(const QPoint& pos);
    void showEdgeContextMenu(const QPoint& pos);

    QHash<int, int> m_nodeIdToRow;
    QHash<QPair<int,int>, int> m_edgeKeyToRow;

    Widgets m_w;
    QHash<int, NetworkNode*>*  m_nodeItems = nullptr;
    QHash<QPair<int,int>, NetworkEdge*>*  m_edgeItems = nullptr;
    DataHandler* m_dataHandler = nullptr;
    NetSim* m_netSimWindow = nullptr;

    bool m_syncingSelection = false;
};

#endif // GRAPHPANEL_H
