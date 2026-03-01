#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H

#include <QObject>
#include <QList>

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

    void setData(const QList<NetworkNode*>& nodes, const QList<NetworkEdge*>& edges);
    void refresh();
    void onGraphSelectionChanged(const QList<QGraphicsItem*>& selectedItems);

signals:
    void tableNodesSelected(QList<NetworkNode*> nodes);
    void tableEdgesSelected(QList<NetworkEdge*> edges);

private slots:
    void showNodeView();
    void showEdgeView();

private:
    void applyStyles();
    void populateNodeTable();
    void populateEdgeTable();
    void updateCountLabels();
    void syncToggleButtons(bool nodesActive);

    Widgets              m_w;
    QList<NetworkNode*>  m_nodes;
    QList<NetworkEdge*>  m_edges;

    bool m_syncingSelection = false;
};

#endif // GRAPHPANEL_H
