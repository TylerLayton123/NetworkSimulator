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

QT_BEGIN_NAMESPACE
namespace Ui {
class NetSim;
}
QT_END_NAMESPACE

// Forward declarations
class NetworkNode;
class NetworkEdge;

/**
 * @class NetworkNode
 * @brief Represents a network device node
 */
class NetworkNode : public QGraphicsEllipseItem {
public:
    NetworkNode(qreal x, qreal y, const QString& label = "", QGraphicsItem* parent = nullptr);
    ~NetworkNode() override = default;
    
    QString label() const { return nodeLabel; }
    void setLabel(const QString& label);
    
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    QString nodeLabel;
    QGraphicsTextItem* labelItem = nullptr;
};

/**
 * @class NetworkEdge
 * @brief Represents a connection between two nodes
 */
class NetworkEdge : public QGraphicsLineItem {
public:
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
    void updateLabelPosition();
};

/**
 * @class NetSim
 * @brief Main application window
 */
class NetSim : public QMainWindow
{
    Q_OBJECT

public:
    NetSim(QWidget *parent = nullptr);
    ~NetSim();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    
private slots:
    void onAddNode();
    void onAddLink();
    void onDeleteSelected();
    void onZoomIn();
    void onZoomOut();
    void onResetView();

private:
    Ui::NetSim *ui = nullptr;
    QGraphicsScene *scene = nullptr;
    QList<NetworkNode*> nodes;
    QList<NetworkEdge*> links;
    
    // Variables for link creation mode
    NetworkNode *linkSourceNode = nullptr;
    bool isCreatingLink = false;
    
    void setupConnections();
    void updateLinks();
    NetworkNode* getNodeAt(const QPointF& pos);
    void createSampleNetwork();
    void onAddNodeAt(const QPointF& position, const QString& label = "");
    void cleanupLinkCreation();  // Added helper function
};

#endif // NETSIM_H