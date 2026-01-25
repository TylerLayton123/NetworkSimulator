#ifndef NETSIM_H
#define NETSIM_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QPointF>
#include <QList>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetSim;
}
QT_END_NAMESPACE

// Simple network node item
class NetworkNode : public QGraphicsEllipseItem {
public:
    NetworkNode(qreal x, qreal y, const QString& label = "", QGraphicsItem* parent = nullptr);
    QString label() const { return nodeLabel; }
    void setLabel(const QString& label);
    
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    QString nodeLabel;
    QGraphicsTextItem* labelItem;
};

// Network link item
class NetworkLink : public QGraphicsLineItem {
public:
    NetworkLink(NetworkNode* source, NetworkNode* destination, QGraphicsItem* parent = nullptr);
    NetworkNode* sourceNode() const { return srcNode; }
    NetworkNode* destNode() const { return dstNode; }
    void updatePosition();
    
private:
    NetworkNode* srcNode;
    NetworkNode* dstNode;
    QGraphicsTextItem* bandwidthLabel;
    void updateLabelPosition();
};

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
    Ui::NetSim *ui;
    QGraphicsScene *scene;
    QList<NetworkNode*> nodes;
    QList<NetworkLink*> links;
    
    // For link creation
    NetworkNode *linkSourceNode;
    bool isCreatingLink;
    
    void setupConnections();
    void updateLinks();
    NetworkNode* getNodeAt(const QPointF& pos);
    void createSampleNetwork();
    void onAddNodeAt(const QPointF& position, const QString& label = "");
};

#endif // NETSIM_H