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
#include <QContextMenuEvent>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>

// ----------------------------------
// NetworkNode implementation
// ----------------------------------

// x and y position, label, and parent graphics object
NetworkNode::NetworkNode(qreal x, qreal y, const QString& label, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-25, -25, 50, 50, parent), nodeLabel(label)
{
    setPos(x, y);
    setBrush(QBrush(Qt::lightGray)); // default fill color
    setPen(QPen(Qt::darkBlue, 2));   // default border color and thickness
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

// Set the label of the node
void NetworkNode::setLabel(const QString& label) {
    nodeLabel = label;
    // if (labelItem) {
    //     labelItem->setPlainText(label);
    // }
}

// draws additional info, might not need this
void NetworkNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QGraphicsEllipseItem::paint(painter, option, widget);
    
    // Draw additional info
    painter->setPen(Qt::darkBlue);
    painter->drawText(boundingRect(), Qt::AlignCenter, nodeLabel);
}

// node is moved
QVariant NetworkNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange || change == ItemPositionHasChanged) {
        // Signal that position changed (connections will update edges)
        emit scene()->changed({ boundingRect() });
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}


// ----------------------------------
// NetworkEdge implementation
// ----------------------------------

// create an edge between source and destination nodes, directed or not
NetworkEdge::NetworkEdge(NetworkNode* source, NetworkNode* destination, bool _directed, const QString& label, QGraphicsItem* parent)
    : QGraphicsLineItem(parent), srcNode(source), dstNode(destination), directed(_directed)
{
    // Validate pointers
    if (!srcNode || !dstNode) {
        qWarning() << "NetworkEdge created with null nodes!";
        return;
    }
    
    // edge color, thickness, ect
    setPen(QPen(Qt::darkGreen, 2, Qt::SolidLine, Qt::RoundCap));
    setLabel(label);
    updatePosition();
}

// set or update the label of an edge
void NetworkEdge::setLabel(const QString& text) {
    // if the label already exists, update it
    if (label) {
        label->setPlainText(text);
    }
    // if not, create it
    else {
        label = new QGraphicsTextItem(text, this);
        label->setDefaultTextColor(Qt::black);
        label->setZValue(1);
    }
    updateLabelPosition();
}

// update the label position and rotation when an edge is moved,
// TODO: need to flip label if angle is upside down
void NetworkEdge::updateLabelPosition() {
    if (!srcNode || !dstNode || !label) return;
    
    QLineF line = this->line();
    QPointF midPoint = line.pointAt(0.5);
    label->setPos(midPoint);
    
    // Calculate angle for label rotation
    qreal angle = std::atan2(line.dy(), line.dx()) * 180 / M_PI;
    label->setRotation(angle);
}

// if a node moves, update the edge position
void NetworkEdge::updatePosition() {
    if (!srcNode || !dstNode) {
        qWarning() << "NetworkEdge::updatePosition: Null node pointers";
        return;
    }
    
    QLineF line(srcNode->pos(), dstNode->pos());
    setLine(line);
    updateLabelPosition();
}

// ----------------------------------
// NetSim implementation
// ----------------------------------

// main window constructor for the netsim application
NetSim::NetSim(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::NetSim), scene(new QGraphicsScene(this)), linkSourceNode(nullptr), isCreatingLink(false)
{
    ui->setupUi(this);
    
    // CRITICAL: Check if graphicsView exists
    if (!ui->graphicsView) {
        qCritical() << "Graphics view not found! Check UI file.";
        QMessageBox::critical(this, "Fatal Error", "Graphics view not initialized.");
        return;
    }
    
    // Set up graphics view settings
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing); // smoother edges
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag); // allow selection rectangle
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // full redraws
    
    // Ensure view accepts mouse events properly
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->setInteractive(true);
    
    // Set scene properties and background color
    scene->setSceneRect(-500, -500, 1000, 1000);
    scene->setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    
    // Connect scene changes to update links
    connect(scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        updateLinks();
    });
    
    // Setup connections
    setupConnections();
    
    // Create sample network
    createSampleNetwork();
    
    // Status bar message
    ui->statusbar->showMessage("Network Simulator Ready. Right-click to add nodes.");
}

NetSim::~NetSim()
{
    // Clean up link creation state
    cleanupLinkCreation();
    
    // Clear all items from scene before deletion
    scene->clear();
    
    // Clear lists (items are owned by scene and will be deleted)
    nodes.clear();
    links.clear();
    
    delete ui;
}

/**
 * @brief Clean up link creation state
 */
void NetSim::cleanupLinkCreation() {
    linkSourceNode = nullptr;
    isCreatingLink = false;
}

void NetSim::setupConnections() {
    // Connect menu actions
    connect(ui->actionAdd_Node, &QAction::triggered, this, &NetSim::onAddNode);
    connect(ui->actionAdd_Link, &QAction::triggered, this, &NetSim::onAddLink);
    connect(ui->actionDelete, &QAction::triggered, this, &NetSim::onDeleteSelected);
    connect(ui->actionZoom_In, &QAction::triggered, this, &NetSim::onZoomIn);
    connect(ui->actionZoom_Out, &QAction::triggered, this, &NetSim::onZoomOut);
    connect(ui->actionReset_View, &QAction::triggered, this, &NetSim::onResetView);
    
    // New action with lambda
    connect(ui->actionNew, &QAction::triggered, this, [this]() {
        cleanupLinkCreation();
        scene->clear();
        nodes.clear();
        links.clear();
        createSampleNetwork();
        ui->statusbar->showMessage("New network created.");
    });
    
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
}

void NetSim::contextMenuEvent(QContextMenuEvent *event) {
    // Check if event is valid
    if (!event) return;
    
    QMenu menu(this);
    
    // Get position in scene coordinates safely
    QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
    QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
    
    // Check if clicked on a node
    NetworkNode* clickedNode = getNodeAt(scenePos);
    
    if (clickedNode) {
        // Node context menu - use actions as children of menu for auto-cleanup
        QAction* addLinkAction = menu.addAction("Add Link from this Node");
        QAction* deleteAction = menu.addAction("Delete Node");
        
        // Store node pointer locally for lambda capture
        NetworkNode* targetNode = clickedNode;
        
        connect(addLinkAction, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            linkSourceNode = targetNode;
            isCreatingLink = true;
            ui->statusbar->showMessage("Click on destination node for the link...");
        });
        
        connect(deleteAction, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            
            // Check if this is the link source node
            if (linkSourceNode == targetNode) {
                cleanupLinkCreation();
                ui->statusbar->showMessage("Link creation cancelled - source node deleted.");
            }
            
            // Remove connected links first
            for (int i = links.size() - 1; i >= 0; i--) {
                NetworkEdge* link = links[i];
                if (link && (link->sourceNode() == targetNode || link->destNode() == targetNode)) {
                    scene->removeItem(link);
                    delete link;
                    links.removeAt(i);
                }
            }
            
            // Remove the node
            nodes.removeOne(targetNode);
            scene->removeItem(targetNode);
            delete targetNode;
        });
    } else {
        // Empty space context menu
        QAction* addNodeAction = menu.addAction("Add Node Here");
        
        // Store scene position for lambda
        QPointF targetPos = scenePos;
        
        connect(addNodeAction, &QAction::triggered, this, [this, targetPos]() {
            onAddNodeAt(targetPos);
        });
    }
    
    menu.exec(event->globalPos());
}

void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Network Node", 
                                         "Node Label:", QLineEdit::Normal, 
                                         QString("Node%1").arg(nodes.size() + 1), &ok);
    
    if (ok && !label.isEmpty()) {
        // Add at center of view safely
        QPoint viewCenter = ui->graphicsView->viewport()->rect().center();
        QPointF scenePos = ui->graphicsView->mapToScene(viewCenter);
        onAddNodeAt(scenePos, label);
    }
}

void NetSim::onAddNodeAt(const QPointF& position, const QString& label) {
    NetworkNode* node = new NetworkNode(position.x(), position.y(), label);
    scene->addItem(node);
    nodes.append(node);
    ui->statusbar->showMessage(QString("Added node: %1").arg(label));
}

void NetSim::onAddLink() {
    if (!linkSourceNode) {
        QMessageBox::information(this, "Add Link", 
                               "Please right-click on a source node first, then select 'Add Link from this Node'");
        return;
    }
    
    isCreatingLink = true;
    ui->statusbar->showMessage("Click on destination node for the link...");
}

/**
 * @brief Handles mouse press events in the main window
 * @param event Mouse event
 */
void NetSim::mousePressEvent(QMouseEvent* event) {
    // CRITICAL FIX: Only handle link creation if we're in that mode
    if (isCreatingLink && event->button() == Qt::LeftButton) {
        // IMPORTANT: Convert coordinates safely
        if (!ui || !ui->graphicsView) {
            cleanupLinkCreation();
            QMainWindow::mousePressEvent(event);
            return;
        }
        
        QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
        QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
        
        NetworkNode* destNode = getNodeAt(scenePos);
        
        if (destNode && destNode != linkSourceNode) {
            // Create link between source and destination
            NetworkEdge* link = new NetworkEdge(linkSourceNode, destNode, false, QString("Link%1").arg(links.size() + 1));
            scene->addItem(link);
            links.append(link);
            
            ui->statusbar->showMessage(QString("Link created between %1 and %2")
                                      .arg(linkSourceNode->label(), destNode->label()));
            
            cleanupLinkCreation();
        } else if (destNode == linkSourceNode) {
            QMessageBox::warning(this, "Invalid Link", 
                               "Cannot create a link from a node to itself.");
            cleanupLinkCreation();
        } else {
            // Clicked on empty space - cancel link creation
            cleanupLinkCreation();
            ui->statusbar->showMessage("Link creation cancelled.");
        }
        
        // IMPORTANT: Don't call parent if we handled the event
        event->accept();
        return;
    }
    
    // Pass other mouse events to parent
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
            // Check if this is the link source node
            if (linkSourceNode == node) {
                cleanupLinkCreation();
                ui->statusbar->showMessage("Link creation cancelled - source node deleted.");
            }
            
            // Remove connected links first
            for (int i = links.size() - 1; i >= 0; i--) {
                NetworkEdge* link = links[i];
                if (link && (link->sourceNode() == node || link->destNode() == node)) {
                    scene->removeItem(link);
                    delete link;
                    links.removeAt(i);
                }
            }
            
            nodes.removeOne(node);
            scene->removeItem(node);
            delete node;
        }
        else if (NetworkEdge* link = dynamic_cast<NetworkEdge*>(item)) {
            links.removeOne(link);
            scene->removeItem(link);
            delete link;
        }
    }
    
    ui->statusbar->showMessage(QString("Deleted %1 item(s)").arg(selectedItems.size()));
}

void NetSim::onZoomIn() {
    ui->graphicsView->scale(1.2, 1.2);
    ui->statusbar->showMessage("Zoomed in");
}

void NetSim::onZoomOut() {
    ui->graphicsView->scale(1/1.2, 1/1.2);
    ui->statusbar->showMessage("Zoomed out");
}

void NetSim::onResetView() {
    ui->graphicsView->resetTransform();
    ui->graphicsView->centerOn(0, 0);
    ui->statusbar->showMessage("View reset");
}

void NetSim::updateLinks() {
    // Update all links when nodes move
    for (NetworkEdge* link : links) {
        if (link) {
            link->updatePosition();
        }
    }
}

NetworkNode* NetSim::getNodeAt(const QPointF& pos) {
    if (!scene) return nullptr;
    
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
    
    // Add to scene and track
    QList<NetworkNode*> sampleNodes = {node1, node2, node3, node4, node5};
    for (NetworkNode* node : sampleNodes) {
        scene->addItem(node);
        nodes.append(node);
    }
    
    // Create sample links
    NetworkEdge* link1 = new NetworkEdge(node1, node2, false, QString("Link1"));
    NetworkEdge* link2 = new NetworkEdge(node2, node3, false, QString("Link2"));
    NetworkEdge* link3 = new NetworkEdge(node2, node4, false, QString("Link3"));
    NetworkEdge* link4 = new NetworkEdge(node2, node5, false, QString("Link4"));
    
    // Add to scene and track
    QList<NetworkEdge*> sampleLinks = {link1, link2, link3, link4};
    for (NetworkEdge* link : sampleLinks) {
        scene->addItem(link);
        links.append(link);
    }
    
    ui->statusbar->showMessage("Sample network created with 5 nodes and 4 links");
}