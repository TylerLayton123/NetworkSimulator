#include "netsim.h"
#include "ui_netsim.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

// NetworkNode implementation
NetworkNode::NetworkNode(qreal x, qreal y, const QString& label, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-25, -25, 50, 50, parent), nodeLabel(label)
{
    setPos(x, y);
    setBrush(QBrush(Qt::lightGray));
    setPen(QPen(Qt::darkBlue, 2));
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    // Create label
    labelItem = new QGraphicsTextItem(label, this);
    labelItem->setPos(-20, -10);
    labelItem->setDefaultTextColor(Qt::black);
}

void NetworkNode::setLabel(const QString& label) {
    nodeLabel = label;
    if (labelItem) {
        labelItem->setPlainText(label);
    }
}

void NetworkNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QGraphicsEllipseItem::paint(painter, option, widget);
    
    // Draw IP address or other info
    painter->setPen(Qt::darkBlue);
    painter->drawText(boundingRect(), Qt::AlignCenter, nodeLabel);
}

QVariant NetworkNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange || change == ItemPositionHasChanged) {
        // The scene will automatically update when items change
        // No need to emit a signal
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

// NetworkLink implementation
NetworkLink::NetworkLink(NetworkNode* source, NetworkNode* destination, QGraphicsItem* parent)
    : QGraphicsLineItem(parent), srcNode(source), dstNode(destination)
{
    setPen(QPen(Qt::darkGreen, 2, Qt::SolidLine, Qt::RoundCap));
    updatePosition();
    
    // Add bandwidth label (optional)
    bandwidthLabel = new QGraphicsTextItem("1 Gbps", this);
    bandwidthLabel->setDefaultTextColor(Qt::darkRed);
    updateLabelPosition();
}

void NetworkLink::updatePosition() {
    if (!srcNode || !dstNode) return;
    
    QLineF line(srcNode->pos(), dstNode->pos());
    setLine(line);
    updateLabelPosition();
}

void NetworkLink::updateLabelPosition() {
    if (!srcNode || !dstNode || !bandwidthLabel) return;
    
    QLineF line = this->line();
    QPointF midPoint = line.pointAt(0.5);
    bandwidthLabel->setPos(midPoint);
    
    // Calculate angle for label rotation
    qreal angle = std::atan2(line.dy(), line.dx()) * 180 / M_PI;
    bandwidthLabel->setRotation(angle);
}

// NetSim implementation
NetSim::NetSim(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetSim)
    , scene(nullptr)
    , linkSourceNode(nullptr)
    , isCreatingLink(false)
{
    ui->setupUi(this);
    
    // Create graphics scene
    scene = new QGraphicsScene(this);
    
    // Set up graphics view
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    
    // Set scene properties
    scene->setSceneRect(-500, -500, 1000, 1000);
    scene->setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    
    // Setup connections
    setupConnections();
    
    // Create sample network for demo
    createSampleNetwork();
    
    // Status bar message
    ui->statusbar->showMessage("Network Simulator Ready. Right-click to add nodes.");
}

NetSim::~NetSim()
{
    delete ui;
    // Scene will be deleted automatically since it has parent
}

void NetSim::setupConnections() {
    // Connect menu actions
    connect(ui->actionAdd_Node, &QAction::triggered, this, &NetSim::onAddNode);
    connect(ui->actionAdd_Link, &QAction::triggered, this, &NetSim::onAddLink);
    connect(ui->actionDelete, &QAction::triggered, this, &NetSim::onDeleteSelected);
    connect(ui->actionZoom_In, &QAction::triggered, this, &NetSim::onZoomIn);
    connect(ui->actionZoom_Out, &QAction::triggered, this, &NetSim::onZoomOut);
    connect(ui->actionReset_View, &QAction::triggered, this, &NetSim::onResetView);
    connect(ui->actionNew, &QAction::triggered, [this]() {
        scene->clear();
        nodes.clear();
        links.clear();
        createSampleNetwork();
    });
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
}

void NetSim::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    
    // Get position in scene coordinates
    QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
    QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
    
    // Check if clicked on a node
    NetworkNode* clickedNode = getNodeAt(scenePos);
    
    if (clickedNode) {
        // Node context menu
        QAction* addLinkAction = new QAction("Add Link from this Node", &menu);
        QAction* deleteAction = new QAction("Delete Node", &menu);
        
        connect(addLinkAction, &QAction::triggered, [this, clickedNode]() {
            linkSourceNode = clickedNode;
            isCreatingLink = true;
            ui->statusbar->showMessage("Click on destination node for the link...");
        });
        
        connect(deleteAction, &QAction::triggered, [this, clickedNode]() {
            // Remove connected links first
            for (int i = links.size() - 1; i >= 0; i--) {
                if (links[i]->sourceNode() == clickedNode || links[i]->destNode() == clickedNode) {
                    scene->removeItem(links[i]);
                    delete links[i];
                    links.removeAt(i);
                }
            }
            
            nodes.removeOne(clickedNode);
            scene->removeItem(clickedNode);
            delete clickedNode;
        });
        
        menu.addAction(addLinkAction);
        menu.addAction(deleteAction);
    } else {
        // Empty space context menu
        QAction* addNodeAction = new QAction("Add Node Here", &menu);
        
        connect(addNodeAction, &QAction::triggered, [this, scenePos]() {
            onAddNodeAt(scenePos);
        });
        
        menu.addAction(addNodeAction);
    }
    
    menu.exec(event->globalPos());
}

void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Network Node", 
                                         "Node Label:", QLineEdit::Normal, 
                                         QString("Node%1").arg(nodes.size() + 1), &ok);
    
    if (ok && !label.isEmpty()) {
        // Add at center of view
        QPointF scenePos = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());
        onAddNodeAt(scenePos, label);
    }
}

void NetSim::onAddNodeAt(const QPointF& position, const QString& label) {
    NetworkNode* node = new NetworkNode(position.x(), position.y(), label);
    scene->addItem(node);
    nodes.append(node);
}

void NetSim::onAddLink() {
    if (!linkSourceNode) {
        QMessageBox::information(this, "Add Link", 
                               "Please right-click on a source node first, then select 'Add Link'");
        return;
    }
    
    isCreatingLink = true;
    ui->statusbar->showMessage("Click on destination node for the link...");
}

void NetSim::mousePressEvent(QMouseEvent* event) {
    if (isCreatingLink && event->button() == Qt::LeftButton) {
        QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
        QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
        
        NetworkNode* destNode = getNodeAt(scenePos);
        
        if (destNode && destNode != linkSourceNode) {
            // Create link between source and destination
            NetworkLink* link = new NetworkLink(linkSourceNode, destNode);
            scene->addItem(link);
            links.append(link);
            
            ui->statusbar->showMessage("Link created successfully.");
            isCreatingLink = false;
            linkSourceNode = nullptr;
        } else if (destNode == linkSourceNode) {
            QMessageBox::warning(this, "Invalid Link", 
                               "Cannot create a link from a node to itself.");
            isCreatingLink = false;
            linkSourceNode = nullptr;
        }
    }
    
    QMainWindow::mousePressEvent(event);
}

void NetSim::onDeleteSelected() {
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "Delete", "No items selected.");
        return;
    }
    
    for (QGraphicsItem* item : selectedItems) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            // Remove connected links first
            for (int i = links.size() - 1; i >= 0; i--) {
                if (links[i]->sourceNode() == node || links[i]->destNode() == node) {
                    scene->removeItem(links[i]);
                    delete links[i];
                    links.removeAt(i);
                }
            }
            
            nodes.removeOne(node);
            scene->removeItem(node);
            delete node;
        }
        else if (NetworkLink* link = dynamic_cast<NetworkLink*>(item)) {
            links.removeOne(link);
            scene->removeItem(link);
            delete link;
        }
    }
}

void NetSim::onZoomIn() {
    ui->graphicsView->scale(1.2, 1.2);
}

void NetSim::onZoomOut() {
    ui->graphicsView->scale(1/1.2, 1/1.2);
}

void NetSim::onResetView() {
    ui->graphicsView->resetTransform();
    ui->graphicsView->centerOn(0, 0);
}

void NetSim::updateLinks() {
    // Update all links (call this when nodes move)
    for (NetworkLink* link : links) {
        link->updatePosition();
    }
}

NetworkNode* NetSim::getNodeAt(const QPointF& pos) {
    QList<QGraphicsItem*> items = scene->items(pos);
    
    for (QGraphicsItem* item : items) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            return node;
        }
    }
    return nullptr;
}

void NetSim::createSampleNetwork() {
    // Create sample network for demonstration
    NetworkNode* node1 = new NetworkNode(-200, -100, "Router1");
    NetworkNode* node2 = new NetworkNode(0, -100, "Switch1");
    NetworkNode* node3 = new NetworkNode(200, -100, "Server1");
    NetworkNode* node4 = new NetworkNode(-100, 100, "PC1");
    NetworkNode* node5 = new NetworkNode(100, 100, "PC2");
    
    scene->addItem(node1);
    scene->addItem(node2);
    scene->addItem(node3);
    scene->addItem(node4);
    scene->addItem(node5);
    
    nodes.append(node1);
    nodes.append(node2);
    nodes.append(node3);
    nodes.append(node4);
    nodes.append(node5);
    
    // Create sample links
    NetworkLink* link1 = new NetworkLink(node1, node2);
    NetworkLink* link2 = new NetworkLink(node2, node3);
    NetworkLink* link3 = new NetworkLink(node2, node4);
    NetworkLink* link4 = new NetworkLink(node2, node5);
    
    scene->addItem(link1);
    scene->addItem(link2);
    scene->addItem(link3);
    scene->addItem(link4);
    
    links.append(link1);
    links.append(link2);
    links.append(link3);
    links.append(link4);
}