#ifndef NETSIM_H
#define NETSIM_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QPointF>
#include <QList>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>
#include <QGraphicsItem>
#include <QScrollBar>
#include <QkeyEvent>
#include <QWheelEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetSim;
}
QT_END_NAMESPACE

// Forward declarations
class NetworkNode;
class NetworkEdge;

// a node on the network
class NetworkNode : public QGraphicsEllipseItem {
public:
    static const int DEFAULT_ZVALUE = 10;
    static const int SELECTED_ZVALUE = 100;

    NetworkNode(qreal x, qreal y, const QString& label = "", QGraphicsItem* parent = nullptr);
    ~NetworkNode() override = default;
    
    QString label() const { return nodeLabel; }
    void setLabel(const QString& label);
    QList<NetworkEdge*> getEdgeList() const { return edgeList; }
    void addEdge(NetworkEdge* edge);
    void addEdge(NetworkNode* otherNode, bool directed, const QString& label);
    void deleteEdge(NetworkEdge* edge);
    
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    QString nodeLabel;
    QList<NetworkEdge*> edgeList; 
};

// an edge connecting two nodes, directed or not
class NetworkEdge : public QGraphicsLineItem {
public:
    static const int DEFAULT_ZVALUE = 0;
    static const int SELECTED_ZVALUE = 5;

    NetworkEdge(NetworkNode* source, NetworkNode* destination, bool directed, const QString& label, QGraphicsItem* parent = nullptr);
    ~NetworkEdge() override = default;
    
    NetworkNode* sourceNode() const { return srcNode; }
    NetworkNode* destNode() const { return dstNode; }
    bool isDirected() const { return directed; }
    void updatePosition();
    void setLabel(const QString& text);
    
private:
    NetworkNode* srcNode = nullptr;
    NetworkNode* dstNode = nullptr;
    bool directed = false;
    QGraphicsTextItem* label = nullptr;
    QGraphicsRectItem* labelBackground = nullptr;
    void updateLabelPosition();
    void updateLabelBackground();
};

// main application window
class NetSim : public QMainWindow
{
    Q_OBJECT

public:
    NetSim(QWidget *parent = nullptr);
    ~NetSim();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
        
// ui action events
private slots:
    void onAddNode();
    void onAddEdge();
    void onDeleteSelected();
    void onZoomIn();
    void onZoomOut();
    void onResetView();
    void onSelectionChanged();

private:
    Ui::NetSim *ui = nullptr;
    QGraphicsScene *scene = nullptr;
    QList<NetworkNode*> nodes;
    QList<NetworkEdge*> edges;

    
    // Variables for edge creation mode
    NetworkNode *edgeSourceNode = nullptr;
    bool isCreatingEdge = false;
    QGraphicsLineItem* tempEdgeLine = nullptr;

    bool isPanning = false;
    QPoint lastPanPoint;
    
    void setupConnections();
    void showContextMenu(const QPoint& viewPos);
    void testGraph();
    void handleZoom(QWheelEvent* event);

    void updateEdges();
    NetworkNode* getNodeAt(const QPointF& pos);
    NetworkNode* AddNodeAt(const QPointF& position, const QString& label = "");
    void AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label);
    void cleanupEdgeCreation();  
    QList<QGraphicsItem*> lastSelectedItems;
};


#endif // NETSIM_H