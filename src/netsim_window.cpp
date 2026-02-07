#include "netsim_classes.h"
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
        setZValue(10);
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

// add an edge to the node's edge list given edge object
void NetworkNode::addEdge(NetworkEdge* edge) {
    edgeList.append(edge);
}

// create edge object and add it to the node's edge list given other node, directed or not, and label
void NetworkNode::addEdge(NetworkNode* otherNode, bool directed, const QString& label) {
    NetworkEdge* edge = new NetworkEdge(this, otherNode, directed, label);
    edgeList.append(edge);
    otherNode->addEdge(edge);
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

    // reset the z values
    labelBackground->setZValue(1);
    label->setZValue(2);
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
    : QMainWindow(parent), ui(new Ui::NetSim), scene(new QGraphicsScene(this)), edgeSourceNode(nullptr), 
    isCreatingEdge(false), lastSelectedItem(nullptr)
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
    ui->graphicsView->viewport()->installEventFilter(this);
    
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
    connect(scene, &QGraphicsScene::selectionChanged, this, &NetSim::onSelectionChanged);

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

    // Get item at click position
    QGraphicsItem* itemAtPos = scene->itemAt(scenePos, QTransform());
    
    // If clicking on an item, select it first
    if (itemAtPos && !itemAtPos->isSelected()) {
        // Clear previous selection
        scene->clearSelection();
        // Select the clicked item
        itemAtPos->setSelected(true);
        // Update z-values
        onSelectionChanged();
    }
    
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
            AddNodeAt(targetPos);
        });
    }
    
    menu.exec(event->globalPos());
}

// ------------------------------
// action events
// ------------------------------

// handles mouse requests
bool NetSim::eventFilter(QObject* watched, QEvent* event) {
    if (watched == ui->graphicsView->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        // Handle edge creation if in that mode
        if (isCreatingEdge && mouseEvent->button() == Qt::LeftButton) {
            QPoint viewPos = mouseEvent->pos();
            QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
            
            NetworkNode* destNode = getNodeAt(scenePos);
            
            if (destNode && destNode != edgeSourceNode) {
                NetworkEdge* edge = new NetworkEdge(edgeSourceNode, destNode, false, 
                    QString("edge%1").arg(edges.size() + 1));
                scene->addItem(edge);
                edges.append(edge);
                
                ui->statusbar->showMessage("Edge created successfully.");
                cleanupEdgeCreation();
            } else if (destNode == edgeSourceNode) {
                QMessageBox::warning(this, "Invalid edge", 
                    "Cannot create an edge from a node to itself.");
                cleanupEdgeCreation();
            }
        }
        
        // Show selection message
        QPoint viewPos = mouseEvent->pos();
        QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
        NetworkNode* clickedNode = getNodeAt(scenePos);
        
        ui->statusbar->showMessage(QString("Clicked on node %1").arg(clickedNode ? clickedNode->label() : "none"));
        
        // Call selection changed handler
        onSelectionChanged();
    }
    
    return QMainWindow::eventFilter(watched, event);
}

// add node action
void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Node", "Node Label:", QLineEdit::Normal, 
        QString("Node%1").arg(nodes.size() + 1), &ok);
    
    // Add at center of view 
    if (ok && !label.isEmpty()) {
        QPoint viewCenter = ui->graphicsView->viewport()->rect().center();
        QPointF scenePos = ui->graphicsView->mapToScene(viewCenter);
        AddNodeAt(scenePos, label);
    }
}

// add node at specific position
NetworkNode* NetSim::AddNodeAt(const QPointF& position, const QString& label) {
    NetworkNode* node = new NetworkNode(position.x(), position.y(), label);
    scene->addItem(node);
    nodes.append(node);
    ui->statusbar->showMessage(QString("Added node: %1").arg(label));
    return node;
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

// add edge to scene given the two, directed or not, and label
void NetSim::AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label) {
    if (!sourceNode || !destNode) return;
    
    // create edge object and add edge to each node
    NetworkEdge* edge = new NetworkEdge(sourceNode, destNode, directed, label);
    sourceNode->addEdge(edge);
    destNode->addEdge(edge);

    scene->addItem(edge);
    edges.append(edge);
    ui->statusbar->showMessage("Edge created successfully.");
}

// delete selected nodes or edges
void NetSim::onDeleteSelected() {
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "Delete", "No items selected.");
        return;
    }

    // Check if lastSelectedItem is being deleted
    bool lastSelectedDeleted = false;
    if (lastSelectedItem && selectedItems.contains(lastSelectedItem)) {
        lastSelectedDeleted = true;
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

    // Reset last selected item if it was deleted
    if (lastSelectedDeleted) {
        lastSelectedItem = nullptr;
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

// when the selceted item moves, node or edge
void NetSim::onSelectionChanged() {
    // Get currently selected items
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    
    // If nothing is selected, reset last selected item
    if (selectedItems.isEmpty()) {
        lastSelectedItem = nullptr;
        return;
    }
    
    // Store the last item in the list (usually the last selected)
    QGraphicsItem* currentLast = selectedItems.last();
    
    // If we have a last selected item that is no longer selected, reset its z-value
    if (lastSelectedItem && lastSelectedItem != currentLast && lastSelectedItem->isSelected()) {
        // Reset z-value based on type
        if (dynamic_cast<NetworkNode*>(lastSelectedItem)) {
            lastSelectedItem->setZValue(10); 
        } else if (dynamic_cast<NetworkEdge*>(lastSelectedItem)) {
            lastSelectedItem->setZValue(0); 
        }
    }
    
    // Set all selected items to high z-value
    const qreal SELECTED_Z = 100;
    for (QGraphicsItem* item : selectedItems) {
        item->setZValue(SELECTED_Z);
    }
    
    // Bring the last selected item to the very top
    const qreal TOP_Z = 101;
    currentLast->setZValue(TOP_Z);
    
    // Update last selected item
    lastSelectedItem = currentLast;
}

// ----------------------------------
// other netsim functions
// ----------------------------------

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
    NetworkNode* node1 = AddNodeAt(QPointF(-200, -100), "A");
    NetworkNode* node2 = AddNodeAt(QPointF(0, -100), "B");
    NetworkNode* node3 = AddNodeAt(QPointF(200, -100), "C");
    NetworkNode* node4 = AddNodeAt(QPointF(-100, 100), "D");
    NetworkNode* node5 = AddNodeAt(QPointF(100, 100), "E");

    // add edges
    AddEdge(node1, node2, false, "-5");
    AddEdge(node2, node3, false, "2");
    AddEdge(node2, node4, false, "3");
    AddEdge(node1, node4, false, "45");
    AddEdge(node4, node5, false, "-47");
    AddEdge(node4, node3, false, "39");

    
    ui->statusbar->showMessage("Sample network created with 5 nodes and 4 edges");
}