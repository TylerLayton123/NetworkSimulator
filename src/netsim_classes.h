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
#include <Qstyle>
#include <QStyleOptionGraphicsItem>
#include <QDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QcheckBox>
#include "graphpanel.h"
#include "algorithmpanel.h"

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

    NetworkEdge(NetworkNode* source, NetworkNode* destination, bool directed, const QString& label, QGraphicsItem* parent = nullptr, bool labelVisible=true);
    ~NetworkEdge() override = default;
    
    NetworkNode* sourceNode() const { return srcNode; }
    NetworkNode* destNode() const { return dstNode; }
    bool isDirected() const { return directed; }
    void updatePosition();
    void setLabel(const QString& text);
    QString getLabel() const { return fullLabelText; }
    void deleteEdge();

    void setLabelVisible(bool visible);
    bool labelVisible = true;
    

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    QPainterPath shape() const override;

private:
    NetworkNode* srcNode = nullptr;
    NetworkNode* dstNode = nullptr;
    bool directed = false;
    QGraphicsTextItem* edgeLabel = nullptr;
    QString fullLabelText;
    QGraphicsRectItem* labelBackground = nullptr;
    void updateLabelPosition();
    void updateLabelBackground();
    // QPointF lastDragPos;
    // bool isDragging = false;
};

// main application window
class NetSim : public QMainWindow
{
    Q_OBJECT

public:
    NetSim(QWidget *parent = nullptr);
    ~NetSim();
    bool multiEdges = false;
    bool directedEdges = false;
    bool showEdgeLabels = true;
    bool showNodeLabels = true;
    bool loopyEdges = false;


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
    void onEditNodeLabel(NetworkNode* targetNode);
    void onEditEdgeLabel(NetworkEdge* clickedEdge);
    void onAddEdgeBtn();
    void onLoadGraph();
    void onViewSettings();

private:
    Ui::NetSim *ui = nullptr;
    QGraphicsScene *scene = nullptr;
    QGraphicsRectItem* sceneBorder = nullptr;
    QList<NetworkNode*> nodes;
    QList<NetworkEdge*> edges;
    GraphPanel* graphPanel = nullptr;
    AlgorithmPanel* algorithmPanel = nullptr;

    
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
    void clearGraph();
    void updateSceneRect();

    void updateEdges();
    NetworkNode* getNodeAt(const QPointF& pos);
    NetworkNode* AddNodeAt(const QPointF& position, const QString& label = "");
    void AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label, bool editLabel = true);
    void cleanupEdgeCreation();  
    QList<QGraphicsItem*> lastSelectedItems;

    
    void deleteEdge(NetworkEdge* edge);
    void deleteNode(NetworkNode* node);
};


#endif // NETSIM_H