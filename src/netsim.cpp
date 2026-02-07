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

    // set nodes above edges
    setZValue(10);
}

// Set the label of the node
void NetworkNode::setLabel(const QString& label) {
    nodeLabel = label;
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

    // draw edges below nodes
    setZValue(0); 
    
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

        // text font
        QFont font = label->font();
        font.setPointSize(8);
        font.setBold(true);
        label->setFont(font);

        // label background that is same color as scene background, allows for better readability when label overlaps edges
        labelBackground = new QGraphicsRectItem(this);
        labelBackground->setBrush(QBrush(QColor(245, 245, 245))); 
        labelBackground->setPen(QPen(Qt::NoPen));

        labelBackground->setZValue(1);
        label->setZValue(2);
    }

    updateLabelBackground();
    updateLabelPosition();
}

// Update the background rectangle to match text size
void NetworkEdge::updateLabelBackground() {
    if (!label || !labelBackground) return;
    
    // Get text bounding rectangle
    QRectF textRect = label->boundingRect();
    
    // Add padding around the text
    qreal padding = 1;
    QRectF backgroundRect = textRect.adjusted(-padding, -padding, padding, padding);
    
    // Set background rectangle
    labelBackground->setRect(backgroundRect);
    
    // Center the text in the background
    label->setPos(backgroundRect.topLeft() + QPointF(padding, padding));
}

// update the label position and rotation when an edge is moved,
void NetworkEdge::updateLabelPosition() {
    if (!srcNode || !dstNode || !label) return;
    
    // new label position at midpoint of edge
    QLineF line = this->line();
    QPointF midPoint = line.pointAt(0.5);
    label->setPos(midPoint - QPointF(label->boundingRect().width() / 2, label->boundingRect().height() / 2));

    // Set position of both text and background
    QRectF bgRect = labelBackground->rect();
    
    // Center the background at the midpoint
    labelBackground->setPos(midPoint - QPointF(bgRect.width() / 2, bgRect.height() / 2));
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
    : QMainWindow(parent), ui(new Ui::NetSim), scene(new QGraphicsScene(this)), edgeSourceNode(nullptr), isCreatingEdge(false)
{
    ui->setupUi(this);
    
    // Check if graphicsView exists
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
    
    // Connect scene changes to update edges
    connect(scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        updateEdges();
    });
    
    // Setup connections between menu and actions
    setupConnections();
    
    // Create sample network for testing
    testGraph();
}

// deconstructor
NetSim::~NetSim()
{
    // Clean up edge creation state
    cleanupEdgeCreation();
    
    // Clear all items from scene before deletion
    scene->clear();
    
    // Clear lists (items are owned by scene and will be deleted)
    nodes.clear();
    edges.clear();
    
    delete ui;
}

// clean up edge creation state
void NetSim::cleanupEdgeCreation() {
    edgeSourceNode = nullptr;
    isCreatingEdge = false;
}

// connect menu actions to their functions
void NetSim::setupConnections() {
    // Connect menu actions
    connect(ui->actionAdd_Node, &QAction::triggered, this, &NetSim::onAddNode);
    connect(ui->actionAdd_Edge, &QAction::triggered, this, &NetSim::onAddEdge);
    connect(ui->actionDelete, &QAction::triggered, this, &NetSim::onDeleteSelected);
    connect(ui->actionZoom_In, &QAction::triggered, this, &NetSim::onZoomIn);
    connect(ui->actionZoom_Out, &QAction::triggered, this, &NetSim::onZoomOut);
    connect(ui->actionReset_View, &QAction::triggered, this, &NetSim::onResetView);

    // new network clears curretn one
    connect(ui->actionNew, &QAction::triggered, this, [this]() {
        cleanupEdgeCreation();
        scene->clear();
        nodes.clear();
        edges.clear();

        // test network
        testGraph();

        ui->statusbar->showMessage("New network created.");
    });
    
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
}

// handles right click context menu events
void NetSim::contextMenuEvent(QContextMenuEvent *event) {
    // Check if event is valid
    if (!event) return;
    
    QMenu menu(this);
    
    // Get position in scene coordinates
    QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
    QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
    
    // Check if clicked on a node
    NetworkNode* clickedNode = getNodeAt(scenePos);
    
    // if on a node show node menu
    if (clickedNode) {
        // Node context menu actions, add edge or delete node
        QAction* addEdgeAction = menu.addAction("Add edge from this Node");
        QAction* deleteAction = menu.addAction("Delete Node");
        
        // node that is clicked on
        NetworkNode* targetNode = clickedNode;
        
        // connect add edge action 
        connect(addEdgeAction, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            edgeSourceNode = targetNode;
            isCreatingEdge = true;
            ui->statusbar->showMessage("Click on destination node for the edge...");
        });
        
        // connect delete action
        connect(deleteAction, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            
            // Remove connected edges first
            for (int i = edges.size() - 1; i >= 0; i--) {
                NetworkEdge* edge = edges[i];
                if (edge && (edge->sourceNode() == targetNode || edge->destNode() == targetNode)) {
                    scene->removeItem(edge);
                    delete edge;
                    edges.removeAt(i);
                }
            }
            
            // Remove the node
            nodes.removeOne(targetNode);
            scene->removeItem(targetNode);
            delete targetNode;
        });
    } else {
        // Empty space context menu
        QAction* addNodeAction = menu.addAction("Add Node");
        
        // Store scene position
        QPointF targetPos = scenePos;
        
        // connect add node action
        connect(addNodeAction, &QAction::triggered, this, [this, targetPos]() {
            onAddNodeAt(targetPos);
        });
    }
    
    menu.exec(event->globalPos());
}

// ------------------------------
// action events
// ------------------------------

// add node action
void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Node", "Node Label:", QLineEdit::Normal, 
        QString("Node%1").arg(nodes.size() + 1), &ok);
    
    // Add at center of view 
    if (ok && !label.isEmpty()) {
        QPoint viewCenter = ui->graphicsView->viewport()->rect().center();
        QPointF scenePos = ui->graphicsView->mapToScene(viewCenter);
        onAddNodeAt(scenePos, label);
    }
}

// add node at specific position
void NetSim::onAddNodeAt(const QPointF& position, const QString& label) {
    NetworkNode* node = new NetworkNode(position.x(), position.y(), label);
    scene->addItem(node);
    nodes.append(node);
    ui->statusbar->showMessage(QString("Added node: %1").arg(label));
}

// add edge action
void NetSim::onAddEdge() {
    if (!edgeSourceNode) {
        QMessageBox::information(this, "Add edge", 
                               "Please right-click on a source node first, then select 'Add edge from this Node'");
        return;
    }
    
    isCreatingEdge = true;
    ui->statusbar->showMessage("Click on destination node for the edge...");
}

// handles mouse press event
void NetSim::mousePressEvent(QMouseEvent* event) {
    // Only handle edge creation if we're in that mode
    if (isCreatingEdge && event->button() == Qt::LeftButton) {
        // Convert coordinates safely
        if (!ui || !ui->graphicsView) {
            cleanupEdgeCreation();
            QMainWindow::mousePressEvent(event);
            return;
        }
        
        QPoint viewPos = ui->graphicsView->mapFromParent(event->pos());
        QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
        
        NetworkNode* destNode = getNodeAt(scenePos);
        
        if (destNode && destNode != edgeSourceNode) {
            // Create edge between source and destination
            NetworkEdge* edge = new NetworkEdge(edgeSourceNode, destNode, false, QString("edge%1").arg(edges.size() + 1));
            scene->addItem(edge);
            edges.append(edge);
            
            ui->statusbar->showMessage("edge created successfully.");
            
            cleanupEdgeCreation();
        // TODO: fix it so there can be self edges
        } else if (destNode == edgeSourceNode) {
            QMessageBox::warning(this, "Invalid edge", 
                               "Cannot create a edge from a node to itself.");
            cleanupEdgeCreation();
        }
    }
    
    // Pass other mouse events to parent
    QMainWindow::mousePressEvent(event);
}

// delete selected nodes or edges
void NetSim::onDeleteSelected() {
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "Delete", "No items selected.");
        return;
    }
    
    // for all of the selected items
    for (QGraphicsItem* item : selectedItems) {
        // delete a node
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {            
            // Remove connected edges first
            for (int i = edges.size() - 1; i >= 0; i--) {
                NetworkEdge* edge = edges[i];
                if (edge && (edge->sourceNode() == node || edge->destNode() == node)) {
                    scene->removeItem(edge);
                    delete edge;
                    edges.removeAt(i);
                }
            }
            
            nodes.removeOne(node);
            scene->removeItem(node);
            delete node;
        }
        // delete an edge
        else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            edges.removeOne(edge);
            scene->removeItem(edge);
            delete edge;
        }
    }
    
    ui->statusbar->showMessage(QString("Deleted %1 item(s)").arg(selectedItems.size()));
}

// zoom in to the scene
void NetSim::onZoomIn() {
    ui->graphicsView->scale(1.2, 1.2);
    ui->statusbar->showMessage("Zoomed in");
}

// zoom out of the scene
void NetSim::onZoomOut() {
    ui->graphicsView->scale(1/1.2, 1/1.2);
    ui->statusbar->showMessage("Zoomed out");
}

// reset view to default
void NetSim::onResetView() {
    ui->graphicsView->resetTransform();
    ui->graphicsView->centerOn(0, 0);
    ui->statusbar->showMessage("View reset");
}

// Update all edges when nodes move
void NetSim::updateEdges() {
    for (NetworkEdge* edge : edges) {
        if (edge) {
            edge->updatePosition();
        }
    }
}

// get the node at a specific position
// TODO: need to fix this to work with right click menu
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

// test graph
void NetSim::testGraph() {
    // Create sample network for demonstration
    NetworkNode* node1 = new NetworkNode(-200, -100, "A");
    NetworkNode* node2 = new NetworkNode(0, -100, "B");
    NetworkNode* node3 = new NetworkNode(200, -100, "C");
    NetworkNode* node4 = new NetworkNode(-100, 100, "D");
    NetworkNode* node5 = new NetworkNode(100, 100, "E");
    
    // Add to scene and track
    QList<NetworkNode*> sampleNodes = {node1, node2, node3, node4, node5};
    for (NetworkNode* node : sampleNodes) {
        scene->addItem(node);
        nodes.append(node);
    }
    
    // Create sample edges
    NetworkEdge* edge1 = new NetworkEdge(node1, node2, false, QString("-5"));
    NetworkEdge* edge2 = new NetworkEdge(node2, node3, false, QString("2"));
    NetworkEdge* edge3 = new NetworkEdge(node2, node4, false, QString("3"));
    NetworkEdge* edge5 = new NetworkEdge(node1, node4, false, QString("45"));
    NetworkEdge* edge6 = new NetworkEdge(node4, node5, false, QString("-47"));
    NetworkEdge* edge7 = new NetworkEdge(node4, node3, false, QString("39"));


    
    // Add to scene and track
    QList<NetworkEdge*> sampleedges = {edge1, edge2, edge3, edge5, edge6, edge7};
    for (NetworkEdge* edge : sampleedges) {
        scene->addItem(edge);
        edges.append(edge);
    }
    
    ui->statusbar->showMessage("Sample network created with 5 nodes and 4 edges");
}