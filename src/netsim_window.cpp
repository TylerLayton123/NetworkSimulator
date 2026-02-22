#include "netsim_classes.h"
#include "ui_netsim.h"

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
    setZValue(NetworkNode::DEFAULT_ZVALUE);
}

// Set the label of the node
void NetworkNode::setLabel(const QString& label) {
    nodeLabel = label;
}

// draws additional info
void NetworkNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Remove the default selection box by clearing the state before passing to base
    QStyleOptionGraphicsItem customOption(*option);
    customOption.state &= ~QStyle::State_Selected;
    QGraphicsEllipseItem::paint(painter, &customOption, widget);
    
    // Draw bright blue border when selected
    if (option->state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor(30, 144, 255), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(boundingRect().adjusted(1, 1, -1, -1));
    }

    // Draw label with truncation
    painter->setPen(Qt::darkBlue);
    qreal availableWidth = boundingRect().width() - 10;
    QFontMetrics fm(painter->font());
    QString displayLabel = fm.elidedText(nodeLabel, Qt::ElideRight, availableWidth);
    painter->drawText(boundingRect(), Qt::AlignCenter, displayLabel);
}

// node is moved
QVariant NetworkNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange || change == ItemPositionHasChanged) {
        // Signal that position changed (connections will update edges)
        emit scene()->changed({ boundingRect() });
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

// delete edge from the edge list
void NetworkNode::deleteEdge(NetworkEdge* edge) {
    edgeList.removeOne(edge);
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
    setZValue(NetworkEdge::DEFAULT_ZVALUE);

    setFlag(QGraphicsItem::ItemIsSelectable);

    
    // edge color, thickness, ect
    setLabel(label);
    updatePosition();
}

// for when an edge is moved move the corrosponding nodes too
void NetworkEdge::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastDragPos = event->scenePos();
    }
    QGraphicsLineItem::mousePressEvent(event);
}

void NetworkEdge::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (isDragging && srcNode && dstNode) {
        QPointF delta = event->scenePos() - lastDragPos;
        lastDragPos = event->scenePos();

        // Collect all nodes that need to move (avoid moving a node twice)
        QSet<NetworkNode*> nodesToMove;

        // Go through all selected items
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
                nodesToMove.insert(node);
            } else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
                if (edge->srcNode) nodesToMove.insert(edge->srcNode);
                if (edge->dstNode) nodesToMove.insert(edge->dstNode);
            }
        }

        // Move all collected nodes by delta
        for (NetworkNode* node : nodesToMove) {
            node->setPos(node->pos() + delta);
        }

        // Update all edges
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
                edge->updatePosition();
            }
        }

        event->accept();
        return;
    }
    QGraphicsLineItem::mouseMoveEvent(event);
}

void NetworkEdge::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    isDragging = false;
    QGraphicsLineItem::mouseReleaseEvent(event);
}

// set or update the label of an edge
void NetworkEdge::setLabel(const QString& text) {
    fullLabelText = text;

    // if the label already exists, update it
    if (edgeLabel) {
        edgeLabel->setPlainText(text);
    }
    // if not, create it
    else {
        edgeLabel = new QGraphicsTextItem(text, this);
        edgeLabel->setDefaultTextColor(Qt::black);

        // text font
        QFont font = edgeLabel->font();
        font.setPointSize(8);
        font.setBold(true);
        edgeLabel->setFont(font);

        // label background that is same color as scene background, allows for better readability when label overlaps edges
        labelBackground = new QGraphicsRectItem(this);
        labelBackground->setBrush(QBrush(QColor(245, 245, 245))); 
        labelBackground->setPen(QPen(Qt::NoPen));

        labelBackground->setZValue(1);
        edgeLabel->setZValue(2);
    }

    updateLabelBackground();
    updateLabelPosition();
}

// Wider invisible hit area to make edges easier to click
QPainterPath NetworkEdge::shape() const {
    QPainterPath path;
    path.moveTo(line().p1());
    path.lineTo(line().p2());
    
    // Stroke the path with a wide pen to create a thick hit area
    QPainterPathStroker stroker;
    stroker.setWidth(16); 
    return stroker.createStroke(path);
}

// Draw edge, bright blue when selected
void NetworkEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    if (!srcNode || !dstNode) return;

    if (isSelected()) {
        painter->setPen(QPen(QColor(30, 144, 255), 3, Qt::SolidLine, Qt::RoundCap));

    } else {
        painter->setPen(QPen(Qt::darkGreen, 2, Qt::SolidLine, Qt::RoundCap));
    }

    painter->drawLine(line());
}

// Update the background rectangle to match text size
void NetworkEdge::updateLabelBackground() {
    if (!edgeLabel || !labelBackground) return;
    
    // Get text bounding rectangle
    QRectF textRect = edgeLabel->boundingRect();
    
    // Add padding around the text
    qreal padding = 1;
    QRectF backgroundRect = textRect.adjusted(-padding, -padding, padding, padding);
    
    // Set background rectangle
    labelBackground->setRect(backgroundRect);
    
    // Center the text in the background
    edgeLabel->setPos(backgroundRect.topLeft() + QPointF(padding, padding));

    // reset the z values
    labelBackground->setZValue(1);
    edgeLabel->setZValue(2);
}

// update the label position and rotation when an edge is moved,
void NetworkEdge::updateLabelPosition() {
    if (!srcNode || !dstNode || !edgeLabel) return;
    
    // new label position at midpoint of edge
    QLineF line = this->line();
    QPointF midPoint = line.pointAt(0.5);

    // qreal edgeLength = line.length();
    // qreal availableWidth = qMax(edgeLength - 80.0, 0.0); 
    // QFontMetrics fm(edgeLabel->font());
    // QString displayText = fm.elidedText(fullLabelText, Qt::ElideRight, availableWidth);
    // edgeLabel->setPlainText(displayText);

    edgeLabel->setPos(midPoint - QPointF(edgeLabel->boundingRect().width() / 2, edgeLabel->boundingRect().height() / 2));
    

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
    isCreatingEdge(false), lastSelectedItems(QList<QGraphicsItem*>()), isPanning(false), lastPanPoint(QPoint())
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
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);

    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Set scene properties and background color
    scene->setSceneRect(-10000, -10000, 20000, 20000);
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
    if (tempEdgeLine) {
        scene->removeItem(tempEdgeLine);
        delete tempEdgeLine;
        tempEdgeLine = nullptr;
    }
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

// New function to show context menu at a specific view position
void NetSim::showContextMenu(const QPoint& viewPos) {
    QMenu menu(this);
    
    // Convert from view coordinates to scene coordinates
    QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
    
    // Check if clicked on a node
    NetworkNode* clickedNode = getNodeAt(scenePos);
    
    // Get item at click position 
    QList<QGraphicsItem*> items = scene->items(scenePos);
    QGraphicsItem* itemAtPos = nullptr;
    if (!items.isEmpty()) {
        itemAtPos = items.first();
    }
    

    // Check if clicked on an edge
    NetworkEdge* clickedEdge = nullptr;
    for (QGraphicsItem* item : items) {
        if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            clickedEdge = edge;
            break;
        }
        // Check if we clicked a child item (label/background) of an edge
        if (item->parentItem()) {
            if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item->parentItem())) {
                clickedEdge = edge;
                break;
            }
        }
    }


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
        QAction* addLabel = menu.addAction("Edit label");
        QAction* addEdgeAction = menu.addAction("Add edge");
        QAction* deleteAction = menu.addAction("Delete Node");
        
        // node that is clicked on
        NetworkNode* targetNode = clickedNode;
        
        // connect add edge action 
        connect(addEdgeAction, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            edgeSourceNode = targetNode;
            isCreatingEdge = true;

            // Create the rubber band line
            tempEdgeLine = new QGraphicsLineItem();
            tempEdgeLine->setPen(QPen(Qt::darkGreen, 2, Qt::SolidLine, Qt::RoundCap));
            tempEdgeLine->setZValue(NetworkEdge::SELECTED_ZVALUE); 
            scene->addItem(tempEdgeLine);
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

        // connect add label action
        connect(addLabel, &QAction::triggered, this, [this, targetNode]() {
            if (!targetNode) return;
            
            bool ok;
            QString newLabel = QInputDialog::getText(
                this,
                "Edit Node Label",
                "Node Label:",
                QLineEdit::Normal,
                targetNode->label(),  
                &ok
            );
            
            if (ok && !newLabel.isEmpty()) {
                targetNode->setLabel(newLabel);
                targetNode->update(); 
                ui->statusbar->showMessage(QString("Node label updated to: %1").arg(newLabel));
            }
        });


    } 
    // can edit or delete edges
    else if (clickedEdge) {
        scene->clearSelection();
        clickedEdge->setSelected(true);
        onSelectionChanged();

        QAction* editWeightAction = menu.addAction("Edit Label");
        QAction* deleteEdgeAction = menu.addAction("Delete Edge");

        connect(editWeightAction, &QAction::triggered, this, [this, clickedEdge]() {
            if (!clickedEdge) return;

            bool ok;
            QString newLabel = QInputDialog::getText(
                this,
                "Edit Edge Label",
                "Edge Label:",
                QLineEdit::Normal,
                clickedEdge->getLabel(),
                &ok
            );

            if (ok) {
                clickedEdge->setLabel(newLabel);
                ui->statusbar->showMessage(QString("Edge label updated to: %1").arg(newLabel));
            }
        });

        connect(deleteEdgeAction, &QAction::triggered, this, [this, clickedEdge]() {
            if (!clickedEdge) return;
            clickedEdge->sourceNode()->deleteEdge(clickedEdge);
            clickedEdge->destNode()->deleteEdge(clickedEdge);
            edges.removeOne(clickedEdge);
            scene->removeItem(clickedEdge);
            delete clickedEdge;
        });
    }
    else {
        scene->clearSelection();

        // Empty space context menu
        QAction* addNodeAction = menu.addAction("Add Node");
        
        // Store scene position
        QPointF targetPos = scenePos;
        
        // connect add node action
        connect(addNodeAction, &QAction::triggered, this, [this, targetPos]() {
            AddNodeAt(targetPos);
        });
    }
    
    // Show menu at the cursor position in global coordinates
    QPoint globalPos = ui->graphicsView->mapToGlobal(viewPos);
    menu.exec(globalPos);
}

// ------------------------------
// action events
// ------------------------------

// handles mouse requests
bool NetSim::eventFilter(QObject* watched, QEvent* event) {
    // left or right mouse button
    if (watched == ui->graphicsView->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        // left mouse click with control or middle mouse click for panning
        if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() & Qt::ControlModifier) ||
            mouseEvent->button() == Qt::MiddleButton) {
            isPanning = true;
            lastPanPoint = mouseEvent->pos();
            ui->graphicsView->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        // normal left click for selection or edge creation
        else if(mouseEvent->button() == Qt::LeftButton && !(mouseEvent->modifiers() & Qt::ControlModifier)){
            // Handle edge creation if in that mode
            if (isCreatingEdge) {
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

            // Call selection changed handler
            onSelectionChanged();
        }
        // Show context menu on right click
        else if (mouseEvent->button() == Qt::RightButton) {
            showContextMenu(mouseEvent->pos());
            return true; 
        }
    }
    // handles mouse moving for panning
    else if (watched == ui->graphicsView->viewport() && event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        // Only pan if we're in panning mode
        if (isPanning) {
            QPoint delta = mouseEvent->pos() - lastPanPoint;
            lastPanPoint = mouseEvent->pos();
            
            // Scroll the view
            QScrollBar* hBar = ui->graphicsView->horizontalScrollBar();
            QScrollBar* vBar = ui->graphicsView->verticalScrollBar();
            hBar->setValue(hBar->value() - delta.x());
            vBar->setValue(vBar->value() - delta.y());
            return true;
        }

        // move the temp edge to follow the mouse when creating an edge
        if (isCreatingEdge && edgeSourceNode && tempEdgeLine) {
            QPointF scenePos = ui->graphicsView->mapToScene(mouseEvent->pos());
            tempEdgeLine->setLine(QLineF(edgeSourceNode->pos(), scenePos));
        }
    }
    // Handle mouse release for panning
    else if (watched == ui->graphicsView->viewport() && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        
        // Stop panning when the panning button is released
        if (isPanning && 
            ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() & Qt::ControlModifier) ||
             mouseEvent->button() == Qt::MiddleButton)) {
            isPanning = false;
            ui->graphicsView->setCursor(Qt::ArrowCursor);
            return true;
        }
    }
    // handles wheel zoom event
    else if (watched == ui->graphicsView->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        ui->statusbar->showMessage(QString("Zooming with wheel, window size %1, %2").arg(ui->graphicsView->width()).arg(ui->graphicsView->height()));
        handleZoom(wheelEvent);
        return true; 
    }
    
    return QMainWindow::eventFilter(watched, event);
}

// escape key cancels edge creation mode
void NetSim::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && isCreatingEdge) {
        cleanupEdgeCreation();
        ui->statusbar->showMessage("Edge creation cancelled.");
    }
    QMainWindow::keyPressEvent(event);
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

    // Remove all lastSelectedItems that are no longer selected
    for (QGraphicsItem* item : lastSelectedItems) {
        if (!selectedItems.contains(item)) {
            lastSelectedItems.removeOne(item);
        }
    }
    
    // for all of the selected items
    for (QGraphicsItem* item : selectedItems) {
        // delete a node
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {            
            // Remove connected edges 
            QList<NetworkEdge*> nodeEdges = node->getEdgeList();
            for (int i = nodeEdges.size() - 1; i >= 0; i--) {
                NetworkEdge* edge = nodeEdges[i];
                scene->removeItem(edge);
                delete edge;
                edges.removeAt(i);
            }
            
            nodes.removeOne(node);
            scene->removeItem(node);
            delete node;
        }
        // delete an edge from both nodes
        else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            edge->sourceNode()->deleteEdge(edge);
            edge->destNode()->deleteEdge(edge);
            edges.removeOne(edge);
            scene->removeItem(edge);
            delete edge;
        }
    }
    
    ui->statusbar->showMessage(QString("Deleted %1 item(s)").arg(selectedItems.size()));
}

// zoom from the wheel event
void NetSim::handleZoom(QWheelEvent* event) {
    const double zoomFactor = 1.15;
    double scaleFactor = 1.0;
    
    // Get zoom direction
    QPoint numDegrees = event->angleDelta();
    
    // pixel scrolling
    if (!numDegrees.isNull()) {
        if (numDegrees.y() > 0) {
            // Zoom in
            scaleFactor = zoomFactor;
        } else {
            // Zoom out
            scaleFactor = 1.0 / zoomFactor;
        }
    }
    
    // Apply scaling
    ui->graphicsView->scale(scaleFactor, scaleFactor);
    event->accept();
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

// reset view be around all items in the scene
// TODO: cache the rect instead of recalculating everytime
void NetSim::onResetView() {
    ui->graphicsView->resetTransform();
    
    if (ui->graphicsView->scene() && ui->graphicsView->scene()->items().count() > 0) {
        QRectF itemsRect = ui->graphicsView->scene()->itemsBoundingRect();
        
        if (!itemsRect.isEmpty()) {
            itemsRect.adjust(-100, -100, 100, 100);
            ui->graphicsView->fitInView(itemsRect, Qt::KeepAspectRatio);
            ui->graphicsView->update();
        } else {
            ui->graphicsView->centerOn(0, 0);
        }
    } else {
        ui->graphicsView->centerOn(0, 0);
    }
    
    ui->statusbar->showMessage("View reset");
}

// when the selected item moves, node or edge
void NetSim::onSelectionChanged() {
    // Get currently selected items
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();

    // if an edge is selected set the nodes to be selected too
    for (QGraphicsItem* item : selectedItems) {
        if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            if (edge->sourceNode() && edge->sourceNode()->scene()) 
                edge->sourceNode()->setSelected(true);
            if (edge->destNode() && edge->destNode()->scene()) 
                edge->destNode()->setSelected(true);
        }
    }

    selectedItems = scene->selectedItems();

    QString lastName = "None";
    if (!selectedItems.isEmpty()) {
        NetworkNode* node = dynamic_cast<NetworkNode*>(selectedItems.last());
        lastName = node ? node->label() : "(edge)";
    }

    ui->statusbar->showMessage(QString("%1 item(s) selected, last: %2, z-value: %3")
        .arg(selectedItems.size())
        .arg(lastName)
        .arg(selectedItems.isEmpty() ? 0 : selectedItems.last()->zValue()));
    
    // Reset ALL previously selected items that are no longer selected
    QList<QGraphicsItem*> itemsToReset;
    
    // reset last selected items that are no longer selected
    for(QGraphicsItem* item : lastSelectedItems) {
        if (!selectedItems.contains(item)) {
            itemsToReset.append(item);
        }
    }
    
    // Reset z-values for items no longer selected
    for (QGraphicsItem* item : itemsToReset) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            item->setZValue(NetworkNode::DEFAULT_ZVALUE);  
            for(NetworkEdge* edge : node->getEdgeList()) {
                edge->setZValue(NetworkEdge::DEFAULT_ZVALUE); 
            }
        } else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            item->setZValue(NetworkEdge::DEFAULT_ZVALUE);   
        }
    }
    
    // If nothing is selected, we're done
    if (selectedItems.isEmpty()) { 
        return; 
    }
    
    // Set all newly selected items to selected z value
    for (QGraphicsItem* item : selectedItems) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            item->setZValue(NetworkNode::SELECTED_ZVALUE);
            for(NetworkEdge* edge : node->getEdgeList()) {
                edge->setZValue(NetworkEdge::SELECTED_ZVALUE);
            }
        } else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            item->setZValue(NetworkEdge::SELECTED_ZVALUE);
        }
    }
    
    // Update last selected item
    lastSelectedItems = selectedItems;
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
NetworkNode* NetSim::getNodeAt(const QPointF& pos) {
    if (!scene) return nullptr;
    
    // Use a small tolerance for picking nodes
    qreal pickTolerance = 10.0; 
    
    // Create a small rectangle around the point for item search
    QRectF pickRect(pos.x() - pickTolerance, pos.y() - pickTolerance, pickTolerance * 2, pickTolerance * 2);
    
    QList<QGraphicsItem*> items = scene->items(pickRect);
    
    // Sort by z-order
    std::sort(items.begin(), items.end(), 
              [](QGraphicsItem* a, QGraphicsItem* b) {
                  return a->zValue() > b->zValue();
              });
    
    for (QGraphicsItem* item : items) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            // Check if point is inside the node's shape
            if (node->shape().contains(node->mapFromScene(pos))) {
                return node;
            }
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