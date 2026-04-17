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
#include <QOpenGLWidget>
#include "graphpanel.h"
#include "datahandler.h"
#include "algorithmpanel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class NetSim;
}
QT_END_NAMESPACE

// Forward declarations
class NetworkNode;
class NetworkEdge;
class GraphPanel;
class DataHandler;
class AlgorithmPanel;

// a node on the network
class NetworkNode : public QGraphicsEllipseItem {
public:
    static const int DEFAULT_ZVALUE = 10;
    static const int SELECTED_ZVALUE = 100;

    // contracted node stats
    static constexpr qreal BASE_RADIUS = 20.0;
    static constexpr qreal RADIUS_PER_NODE = 3.5;
    static constexpr qreal MAX_RADIUS = 72.0;

    NetworkNode(qreal x, qreal y, const QString& label = "", QGraphicsItem* parent = nullptr);
    ~NetworkNode() override = default;
    
    QString getLabel() const { return fullLabelText; }
    void setLabel(const QString& label);

    int nodeFrontId = -1;

    // contraction functions
    void setContracted(const QVector<int>& memberIds);
    bool isContracted() const { return m_contracted; }
    int memberCount() const { return m_memberFrontIds.size(); }
    const QVector<int>& memberFrontIds() const { return m_memberFrontIds; }
    qreal contractedRadius() const { return m_contractedRadius; }

    void registerEdge(NetworkEdge* e)   { m_edges.insert(e); }
    void unregisterEdge(NetworkEdge* e) { m_edges.remove(e); }
    
    
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QString fullLabelText;

    QSet<NetworkEdge*> m_edges;

    bool m_contracted = false;
    QVector<int> m_memberFrontIds;
    qreal m_contractedRadius = BASE_RADIUS;
};

// an edge connecting two nodes, directed or not
class NetworkEdge : public QGraphicsLineItem {
public:
    static const int DEFAULT_ZVALUE = 0;
    static const int SELECTED_ZVALUE = 5;

    NetworkEdge(NetworkNode* source, NetworkNode* destination, bool directed, const QString& label, QGraphicsItem* parent = nullptr, bool labelVisible=true);
    ~NetworkEdge() {
        if (srcNode) srcNode->unregisterEdge(this);
        if (dstNode) dstNode->unregisterEdge(this);
    }
    
    NetworkNode* sourceNode() const { return srcNode; }
    NetworkNode* destNode() const { return dstNode; }
    bool isDirected() const { return directed; }
    void updatePosition();
    void setLabel(const QString& text);
    QString getLabel() const { return fullLabelText; }
    void deleteEdge();

    void setContracted(bool contracted, int count = 1, int totalNodes = 10);
    bool isContractedEdge() const { return m_contractedEdge; }
    int contractedCount() const { return m_contractedCount; }

    void setLabelVisible(bool visible);
    bool labelVisible = true;
    

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    QPainterPath shape() const override;

private:
    NetworkNode* srcNode = nullptr;
    NetworkNode* dstNode = nullptr;
    bool directed = false;
    bool m_contractedEdge = false;
    int m_contractedCount = 1;
    int m_totalNodes = 10;
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

    GraphPanel* graphPanel = nullptr;

    int registerContractedNode(NetworkNode* contracted, const QVector<int>& memberFrontIds);
    void setNodeContractedMapping(int backendNodeId, int nodeFrontId);
    
    const QVector<int> getMembers(int frontId) const{
        return m_contractedMembers.value(frontId);
    };

    void updateSceneRect();
    void clearLastItems() { lastSelectedItems.clear();}
    void resetView() {onResetView();};
    void resetFrontendState();

    int backIdToFrontId(int backId) const { return m_backIdToFrontId.value(backId, backId); }
    void setBackIdToFrontId(int backId, int frontId) { m_backIdToFrontId[backId] = frontId; }
    void AddVisualEdge(int srcFrontId, int dstFrontId, const QString& label, bool directed=false);


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
    void onExpandNode(NetworkNode* contractedNode);
    void onContractSelected();
    

private:
    Ui::NetSim *ui = nullptr;
    QGraphicsScene *scene = nullptr;
    QGraphicsRectItem* sceneBorder = nullptr;
    
    AlgorithmPanel* algorithmPanel = nullptr;

    QHash<int, int> m_backIdToFrontId;
    QHash<int, QVector<int>> m_contractedMembers;
    int m_nextContractedId = -1; 
    

    DataHandler* dataHandler = nullptr;
    QHash<int, NetworkNode*> nodeItems;
    QHash<QPair<int,int>, NetworkEdge*> edgeItems;

    
    // Variables for edge creation mode
    NetworkNode *edgeSourceNode = nullptr;
    bool isCreatingEdge = false;
    QGraphicsLineItem* tempEdgeLine = nullptr;

    bool isPanning = false;
    QPoint lastPanPoint;
    bool updatingEdges = false;

    QString m_defaultLayoutAlgo = "none";
    
    void setupConnections();
    void setupViewport();
    void showContextMenu(const QPoint& viewPos);
    void testGraph();
    void handleZoom(QWheelEvent* event);
    void clearGraph();
    

    void updateEdges();
    NetworkNode* getNodeAt(const QPointF& pos);
    NetworkNode* AddNodeAt(const QPointF& position, const QString& label = "", int initialCapacity = 4, int nodeId = -1);
    void AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label, bool editLabel = true);
    void cleanupEdgeCreation();  
    QList<QGraphicsItem*> lastSelectedItems;

    
    void deleteEdge(NetworkEdge* edge);
    void deleteNode(NetworkNode* node);
};


#endif // NETSIM_H