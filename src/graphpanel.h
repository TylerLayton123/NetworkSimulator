#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H

#include <QObject>
#include <QList>
#include "datahandler.h"

class NetworkNode;
class NetworkEdge;
class QGraphicsItem;

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
        QLabel*         titleLbl     = nullptr;
        QSplitter*      splitter     = nullptr; 
    };

    explicit GraphPanel(const Widgets& w, QObject* parent = nullptr);

    void setData(QHash<int, NetworkNode*>* nodes, QHash<QPair<int,int>, NetworkEdge*>* edges, DataHandler* dataHandler);
    void refresh();
    void addNodeRow(int nodeId);
    void removeNodeRow(int nodeId);
    void addEdgeRow(int srcId, int dstId);
    void removeEdgeRow(int srcId, int dstId);
    void onGraphSelectionChanged(const QList<QGraphicsItem*>& selectedItems);
    void updateNodePositions();

signals:
    void tableNodesSelected(QHash<int, NetworkNode*>& nodes);
    void tableEdgesSelected(QHash<QPair<int,int>, NetworkEdge*>& edges);

private slots:
    void showNodeView();
    void showEdgeView();

private:
    void applyStyles();
    void populateNodeTable();
    void populateEdgeTable();
    void updateCountLabels();
    void syncToggleButtons(bool nodesActive);

    Widgets m_w;
    QHash<int, NetworkNode*>*  m_nodeItems = nullptr;
    QHash<QPair<int,int>, NetworkEdge*>*  m_edgeItems = nullptr;
    DataHandler* m_dataHandler = nullptr;

    bool m_syncingSelection = false;
};

#endif // GRAPHPANEL_H
