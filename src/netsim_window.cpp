#include "netsim_classes.h"
#include "ui_netsim.h"
#include <QTimer>

// ----------------------------------
// NetworkNode implementation
// ----------------------------------

// x and y position, label, and parent graphics object
NetworkNode::NetworkNode(qreal x, qreal y, const QString& label, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-25, -25, 50, 50, parent)
{
    fullLabelText = label;
    setPos(x, y);
    setBrush(QBrush(Qt::lightGray)); // default fill color
    setPen(QPen(Qt::darkBlue, 2));   // default border color and thickness
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    // set nodes above edges
    setZValue(NetworkNode::DEFAULT_ZVALUE);
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
    QString displayLabel = fm.elidedText(getLabel(), Qt::ElideRight, availableWidth);
    painter->drawText(boundingRect(), Qt::AlignCenter, displayLabel);
}

// node is moved
QVariant NetworkNode::itemChange(GraphicsItemChange change, const QVariant &value) {

    // make sure the node stays within the scene rect when moved
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        QRectF bounds = scene()->sceneRect();
        const qreal radius = 25.0; 

        // Clamp so the node circle stays fully inside the scene rect
        newPos.setX(qBound(bounds.left()  + radius, newPos.x(), bounds.right()  - radius));
        newPos.setY(qBound(bounds.top()   + radius, newPos.y(), bounds.bottom() - radius));

        return newPos; 
    }

    if (change == ItemPositionHasChanged) {
        // Signal that position changed (connections will update edges)
        emit scene()->changed({ boundingRect() });
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

// set the label of a nodes item and update the display
void NetworkNode::setLabel(const QString& label) {
    fullLabelText = label;
    update();
}

// ----------------------------------
// NetworkEdge implementation
// ----------------------------------

// create an edge between source and destination nodes, directed or not
NetworkEdge::NetworkEdge(NetworkNode* source, NetworkNode* destination, bool _directed, const QString& label, QGraphicsItem* parent, bool _labelVisible)
    : QGraphicsLineItem(parent), srcNode(source), dstNode(destination), directed(_directed), labelVisible(_labelVisible)
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
    if(labelVisible) {
        setLabel(label);
    }
    else {
        fullLabelText = label;
    }
    updatePosition();
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

// delete the edges label or create it
void NetworkEdge::setLabelVisible(bool visible) {
    labelVisible = visible;
    if (!visible) {
        if (edgeLabel) {
            delete edgeLabel;       
            edgeLabel = nullptr; 
        }
        if (labelBackground) { 
            delete labelBackground; 
            labelBackground = nullptr; 
        }
    } else {
        if (!edgeLabel) {
            setLabel(fullLabelText);
        }
    }
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
    dataHandler = new DataHandler();
    
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
    // ui->graphicsView->setViewport(new QOpenGLWidget());
    setupViewport();
    
    // Ensure view accepts mouse events properly
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->setInteractive(true);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);

    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->toolBar->setMovable(false);
    
    // Set scene properties and background color
    scene->setSceneRect(-10000, -10000, 20000, 20000);
    scene->setBackgroundBrush(QBrush(QColor(245, 245, 245)));

    // border on the scene
    sceneBorder = new QGraphicsRectItem();
    sceneBorder->setPen(QPen(QColor(180, 180, 180), 10));
    sceneBorder->setBrush(Qt::NoBrush);
    sceneBorder->setZValue(-1); 
    scene->addItem(sceneBorder);
    updateSceneRect();
    
    // Connect scene changes to update edges
    connect(scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        updateEdges();
    });
    
    // Setup connections between menu and actions
    setupConnections();
    
    // Create sample network for testing
    testGraph();

    GraphPanel::Widgets pw;
    pw.nodePanelBtn = ui->nodePanelBtn;
    pw.edgePanelBtn = ui->edgePanelBtn;
    pw.nodeTable    = ui->nodeTable;
    pw.edgeTable    = ui->edgeTable;
    pw.panelStack   = ui->panelStack;
    pw.nodeCountLbl = ui->nodeCountLabel;
    pw.edgeCountLbl = ui->edgeCountLabel;
    pw.titleLbl     = ui->panelTitleLabel;
    pw.splitter     = ui->mainSplitter;

    graphPanel = new GraphPanel(pw, this);
    if (graphPanel) graphPanel->setData(&nodeItems, &edgeItems, dataHandler);

    // send signal after entire selection is finalized:
    connect(scene, &QGraphicsScene::selectionChanged, this, [this]() {
        QTimer::singleShot(0, this, [this]() {
            if (graphPanel) graphPanel->onGraphSelectionChanged(scene->selectedItems());
        });
    });

    // connect signals from graph panel when table selection changes to update the scene selection
    connect(graphPanel, &GraphPanel::tableNodesSelected, this, [this](QHash<int, NetworkNode*> selectedNodes) {
        scene->clearSelection();
        for (NetworkNode* node : selectedNodes)
            node->setSelected(true);
    });

    connect(graphPanel, &GraphPanel::tableEdgesSelected, this, [this](QHash<QPair<int,int>, NetworkEdge*> selectedEdges) {
    scene->clearSelection();
        scene->clearSelection();
        for (NetworkEdge* edge : selectedEdges)
            edge->setSelected(true);
    });


    ui->panelToolbar->setStyleSheet(
        "border-bottom: 1px solid #b0b8c8;");

    ui->graphInfoPanel->setStyleSheet(
        "QWidget#graphInfoPanel {"
        "  border: 1px solid #b0b8c8;"
        "  border-bottom: none;"   
        "  margin: 0px 4px 0px 4px;"  
        "}");

    // set the algorithm panel
    algorithmPanel = new AlgorithmPanel(ui->algoPanelContainer);
    auto* lay = new QVBoxLayout(ui->algoPanelContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(algorithmPanel);
    if (algorithmPanel) algorithmPanel->setData(&nodeItems, &edgeItems, dataHandler);
    ui->topSplitter->setSizes({800, 500});

}

// deconstructor
NetSim::~NetSim()
{
    // Clean up edge creation state
    cleanupEdgeCreation();
    
    // Clear all items from scene before deletion
    scene->clear();
    
    // clear the edge and node maps
    nodeItems.clear();
    edgeItems.clear();

    delete dataHandler;
    delete ui;
}

// dynamically expand the scene when nodes are added outside current space
void NetSim::updateSceneRect() {
    const qreal margin = 2000.0;

    QRectF minRect(-5000, -5000, 10000, 10000);
    QRectF finalRect = minRect;

    if (!nodeItems.isEmpty()) {
        QRectF nodeBounds;
        for (NetworkNode* node : nodeItems.values()) {
            QRectF r = node->mapToScene(node->boundingRect()).boundingRect();
            nodeBounds = nodeBounds.isNull() ? r : nodeBounds.united(r);
        }
        finalRect = nodeBounds.adjusted(-margin, -margin, margin, margin).united(minRect);
    }

    scene->setSceneRect(finalRect);

    // readjust border
    if (sceneBorder)
        sceneBorder->setRect(finalRect.adjusted(3, 3, -3, -3)); 
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
    connect(ui->actionViewSettings, &QAction::triggered, this, &NetSim::onViewSettings);
    connect(ui->actionZoom_In, &QAction::triggered, this, &NetSim::onZoomIn);
    connect(ui->actionZoom_Out, &QAction::triggered, this, &NetSim::onZoomOut);
    connect(ui->actionReset_View, &QAction::triggered, this, &NetSim::onResetView);

    connect(scene, &QGraphicsScene::selectionChanged, this, [this]() {
        QTimer::singleShot(0, this, &NetSim::onSelectionChanged);
    });

    connect(ui->panelAddNodeBtn,  &QPushButton::clicked, this, &NetSim::onAddNode);
    connect(ui->panelAddEdgeBtn,  &QPushButton::clicked, this, &NetSim::onAddEdgeBtn);
    connect(ui->panelDeleteBtn,   &QPushButton::clicked, this, &NetSim::onDeleteSelected);

    // connect the load action
    connect(ui->actionLoad, &QAction::triggered, this,  &NetSim::onLoadGraph);

    // new network clears current one
    connect(ui->actionNew, &QAction::triggered, this, [this]() {
        clearGraph();
        ui->statusbar->showMessage("New network created.");
    });
    
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
}

// set up the viewport
void NetSim::setupViewport() {
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->viewport()->setMouseTracking(true);
    ui->graphicsView->viewport()->setFocusPolicy(Qt::StrongFocus);
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->setInteractive(true);
    ui->graphicsView->setFocusPolicy(Qt::StrongFocus);
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

    // determine which was clicked on
    QGraphicsItem* selectableItem = nullptr;
    if (clickedNode) {
        selectableItem = clickedNode;
    } else if (clickedEdge) {
        selectableItem = clickedEdge;
    } else if (itemAtPos) {
        QGraphicsItem* item = itemAtPos;
        while (item && !item->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
            item = item->parentItem();
        }
        selectableItem = item;
    }

    // if not selected, select it and clear others
    if (selectableItem) {
        if (!selectableItem->isSelected()) {
            scene->clearSelection();
            selectableItem->setSelected(true);
            onSelectionChanged();
        }
    } else {
        // Clicked empty space - clear selection
        scene->clearSelection();
    }

    
    // if on a node show node menu
    if (clickedNode) {
        // Node context menu actions, add edge or delete node
        QAction* addLabel = menu.addAction("Edit label");
        QAction* addEdgeAction = menu.addAction("Add edge");

        QAction* deleteAction = menu.addAction("Delete");
        
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
            onDeleteSelected();
        });

        // connect add label action
        connect(addLabel, &QAction::triggered, this, [this, targetNode]() {
            onEditNodeLabel(targetNode);
        });
    } 
    // can edit or delete edges
    else if (clickedEdge) {
        // scene->clearSelection();
        clickedEdge->setSelected(true);
        // onSelectionChanged();

        QAction* editWeightAction = menu.addAction("Edit Label");
        QAction* deleteEdgeAction = menu.addAction("Delete");

        connect(editWeightAction, &QAction::triggered, this, [this, clickedEdge]() {
            onEditEdgeLabel(clickedEdge);
        });

        connect(deleteEdgeAction, &QAction::triggered, this, [this, clickedEdge]() {
            onDeleteSelected();
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
            bool ok;
            QString label = QInputDialog::getText(this, "Add Node", "Node Label:", QLineEdit::Normal, 
                QString("Node%1").arg(nodeItems.size() + 1), &ok);
            
            // Add node at clicked position
            if (ok && !label.isEmpty()) {
                AddNodeAt(targetPos, label);
            }
            
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
        
        // left mouse click with shift or middle mouse click for panning
        if ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() & Qt::ShiftModifier) || 
            mouseEvent->button() == Qt::MiddleButton) {
            isPanning = true;
            lastPanPoint = mouseEvent->pos();
            ui->graphicsView->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        // normal left click for selection or edge creation
        else if(mouseEvent->button() == Qt::LeftButton && !(mouseEvent->modifiers() & Qt::ShiftModifier)){
            // Handle edge creation if in that mode
            if (isCreatingEdge) {
                QPoint viewPos = mouseEvent->pos();
                QPointF scenePos = ui->graphicsView->mapToScene(viewPos);
                
                NetworkNode* destNode = getNodeAt(scenePos);
                
                // if the node exists
                if (destNode && destNode != edgeSourceNode) {
                    // check if the edge already exists between the two nodes
                    int srcId = edgeSourceNode->nodeId;
                    int dstId = destNode->nodeId;
                    bool edgeExists = dataHandler->edgeExists(srcId, dstId);
                    if (!directedEdges && !edgeExists) {
                        edgeExists = dataHandler->edgeExists(dstId, srcId);
                    }

                    // for multi edges
                    if (edgeExists && !multiEdges) {
                        QMessageBox::warning(this, "Invalid edge", 
                            "An edge already exists between these nodes. Multi-edges not allowed.");
                        cleanupEdgeCreation();
                        return true;
                    }

                    AddEdge(edgeSourceNode, destNode, false, QString("edge%1").arg(edgeItems.size() + 1));

                    cleanupEdgeCreation();
                    
                    ui->statusbar->showMessage("Edge created successfully.");
                } 
                // handle loopy edges
                else if (destNode == edgeSourceNode) {
                    if(loopyEdges) {
                        AddEdge(edgeSourceNode, destNode, false, QString("edge%1").arg(edgeItems.size() + 1));
                    
                        ui->statusbar->showMessage("Edge created successfully.");
                        cleanupEdgeCreation();
                    } 
                    else {
                        QMessageBox::warning(this, "Invalid edge", 
                            "Loopy edges not allowed.");
                        cleanupEdgeCreation();
                    }
                } else {
                    QMessageBox::warning(this, "Invalid edge", 
                        "Please click on a valid destination node to create the edge.");
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
            ((mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() & Qt::ShiftModifier) ||
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
    // delete key deletes selected items
    else if(event->key() == Qt::Key_Delete) {
        onDeleteSelected();
    }

    QMainWindow::keyPressEvent(event);
}

// clear the entire graph
void NetSim::clearGraph() {
    cleanupEdgeCreation();
    lastSelectedItems.clear();
    scene->clearSelection();

    // simply delete the backend and create a new one to clear it
    delete dataHandler;
    dataHandler = new DataHandler();

    nodeItems.clear();
    edgeItems.clear();

    scene->clear();
    sceneBorder = nullptr; 

    // Re-create the border 
    sceneBorder = new QGraphicsRectItem();
    sceneBorder->setPen(QPen(QColor(180, 180, 180), 10));
    sceneBorder->setBrush(Qt::NoBrush);
    sceneBorder->setZValue(-1);
    scene->addItem(sceneBorder);

    updateSceneRect();
}

// add node action
void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Node", "Node Label:", QLineEdit::Normal, 
        QString("Node%1").arg(nodeItems.size() + 1), &ok);
    
    // Add at center of view 
    if (ok && !label.isEmpty()) {
        QPoint viewCenter = ui->graphicsView->viewport()->rect().center();
        QPointF scenePos = ui->graphicsView->mapToScene(viewCenter);
        AddNodeAt(scenePos, label);
    }
}

// add node at specific position
NetworkNode* NetSim::AddNodeAt(const QPointF& position, const QString& label) {
    // add node to nackend
    int newNodeId = dataHandler->addNode(label);

    // add node to scene
    NetworkNode* node = new NetworkNode(position.x(), position.y(), label);
    node->nodeId = newNodeId;
    scene->addItem(node);
    nodeItems[newNodeId] = node;

    if(graphPanel) graphPanel->addNodeRow(newNodeId);

    updateSceneRect();
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

// add an edge from the graph panel
void NetSim::onAddEdgeBtn() {
    if (nodeItems.size() < 2) {
        QMessageBox::information(this, "Add Edge", "Need at least 2 nodes to create an edge.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Add Edge");
    dlg.setMinimumWidth(280);

    auto* form   = new QFormLayout;
    auto* layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);

    // Source combo
    auto* sourceCbo = new QComboBox;
    for (NetworkNode* n : nodeItems.values())
        sourceCbo->addItem(n->getLabel(), QVariant::fromValue(static_cast<void*>(n)));
    form->addRow("Source:", sourceCbo);

    // Destination combo
    auto* destCbo = new QComboBox;
    for (NetworkNode* n : nodeItems.values())
        destCbo->addItem(n->getLabel(), QVariant::fromValue(static_cast<void*>(n)));
    // Default dest to second node so source != dest on open
    if (destCbo->count() > 1) destCbo->setCurrentIndex(1);
    form->addRow("Destination:", destCbo);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    auto* src  = static_cast<NetworkNode*>(sourceCbo->currentData().value<void*>());
    auto* dest = static_cast<NetworkNode*>(destCbo->currentData().value<void*>());

    if (!src || !dest) return;

    if (src == dest) {
        if (!loopyEdges) {
            QMessageBox::warning(this, "Invalid Edge", "Loopy edges are not allowed.");
            return;
        }
    }

    // Check for duplicate edge
    if (!multiEdges) {
        int srcId = src->nodeId;
        int dstId = dest->nodeId;
        if (dataHandler->edgeExists(srcId, dstId) || (!directedEdges && dataHandler->edgeExists(dstId, srcId))) {
            QMessageBox::warning(this, "Invalid Edge",
                "An edge already exists between these nodes. Multi-edges not allowed.");
            return;
        }
    }

    AddEdge(src, dest, false, QString("edge%1").arg(edgeItems.size() + 1));
}

// edit node label action
void NetSim::onEditNodeLabel(NetworkNode* targetNode) {
    if (!targetNode) return;
    
    bool ok;
    QString newLabel = QInputDialog::getText(
        this,
        "Edit Node Label",
        "Node Label:",
        QLineEdit::Normal,
        targetNode->getLabel(),  
        &ok
    );
    
    if (ok && !newLabel.isEmpty()) {
        int nodeId = targetNode->nodeId;
        dataHandler->setNodeLabel(nodeId, newLabel);
        nodeItems.value(nodeId)->setLabel(newLabel);
        targetNode->update(); 
        ui->statusbar->showMessage(QString("Node label updated to: %1").arg(newLabel));
    }
}

void NetSim::onEditEdgeLabel(NetworkEdge* clickedEdge) {
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
        dataHandler->setEdgeLabel(clickedEdge->sourceNode()->nodeId, clickedEdge->destNode()->nodeId, newLabel);
        ui->statusbar->showMessage(QString("Edge label updated to: %1").arg(newLabel));
    }
}

// add edge to scene given the two, directed or not, and label
void NetSim::AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label, bool editLabel) {
    if (!sourceNode || !destNode) return;

    int srcId = sourceNode->nodeId;
    int dstId = destNode->nodeId;

    // add edge to the data handler
    dataHandler->addEdge(srcId, dstId, label);
    if (!directedEdges) {
        dataHandler->addEdge(dstId, srcId, label);
    }
    
    // create edge object and add edge to each node
    NetworkEdge* edge = new NetworkEdge(sourceNode, destNode, directed, label, nullptr, showEdgeLabels);

    scene->addItem(edge);
    edgeItems[QPair<int,int>(srcId, dstId)] = edge;
    if (!directedEdges) {
        edgeItems[QPair<int,int>(dstId, srcId)] = edge;
    }
    ui->statusbar->showMessage("Edge created successfully.");

    // remove temp line
    cleanupEdgeCreation();

    if (editLabel) {
        onEditEdgeLabel(edge);
    }

    // add row to panel
    if(graphPanel) graphPanel->addEdgeRow(srcId, dstId);
}


// delete an edge from the scene and both nodes
void NetSim::deleteEdge(NetworkEdge* edge) {
    if (!edge) return;

    // get the edge nodes ids
    NetworkNode* src = edge->sourceNode();
    NetworkNode* dst = edge->destNode();
    int srcId = src->nodeId;
    int dstId = dst->nodeId;

    // Remove from backend
    dataHandler->removeEdge(srcId, dstId);
    if (!directedEdges) {
        dataHandler->removeEdge(dstId, srcId);
    }

    // Remove from map
    edgeItems.remove(QPair<int,int>(srcId, dstId));
    if (!directedEdges) {
        edgeItems.remove(QPair<int,int>(dstId, srcId));
    }

    lastSelectedItems.removeOne(edge);

    scene->removeItem(edge);
    delete edge;
}

// delete node from the scene and all connected edges
void NetSim::deleteNode(NetworkNode* node) {
    if (!node) return;
    int nodeId = node->nodeId;

    lastSelectedItems.removeOne(node);

    // Get all edges incident to this node from backend
    const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(nodeId);
    for (const EdgeInfo& e : incidentEdges) {
        int neighborId = e.destination;

        // Look up the visual edge
        NetworkEdge* edge = edgeItems.value(QPair<int,int>(nodeId, neighborId));
        if (!edge && !directedEdges) {
            edge = edgeItems.value(QPair<int,int>(neighborId, nodeId));
        }
        if (edge) {
            deleteEdge(edge);
        }
    }

    // removes a node with no edges so we dont have to loop through edges
    dataHandler->removeNodeNoEdges(nodeId);

    // remove item node
    nodeItems.remove(nodeId);
    scene->removeItem(node);
    delete node;
}

// delete all selected items
void NetSim::onDeleteSelected() {
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "Delete", "No items selected.");
        return;
    }

    // Separate into nodes and edges BEFORE deleting anything
    QList<NetworkNode*> selectedNodes;
    QList<NetworkEdge*> selectedEdges;

    // seperate selected items
    for (QGraphicsItem* item : selectedItems) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item))
            selectedNodes.append(node);
        else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item))
            selectedEdges.append(edge);
    }

    // collect edge keys before any deletion so we don't dereference deleted pointers
    QList<QPair<int,int>> edgeKeysToDelete;
    for (NetworkEdge* edge : selectedEdges) {
        QPair<int,int> key = {qMin(edge->sourceNode()->nodeId, edge->destNode()->nodeId),
                              qMax(edge->sourceNode()->nodeId, edge->destNode()->nodeId)};
        edgeKeysToDelete.append(key);
    }

    // First delete nodes and their incident edges
    for (NetworkNode* node : selectedNodes) {
        int nodeId = node->nodeId;

        // capture incident edge keys BEFORE deleteNode removes them
        const QVector<EdgeInfo> incident = dataHandler->getEdgesOf(nodeId);
        QList<QPair<int,int>> incidentKeys;
        for (const EdgeInfo& e : incident)
            incidentKeys.append({qMin(nodeId, e.destination), qMax(nodeId, e.destination)});

        deleteNode(node);

        // remove node row and all incident edge rows from the panel
        if (graphPanel) {
            graphPanel->removeNodeRow(nodeId);
            for (const QPair<int,int>& key : incidentKeys)
                graphPanel->removeEdgeRow(key.first, key.second);
        }
    }

    // Second delete any explicitly selected edges not already deleted by deleteNode
    for (const QPair<int,int>& key : edgeKeysToDelete) {
        if (edgeItems.contains(key)) {
            deleteEdge(edgeItems.value(key));
            if (graphPanel) graphPanel->removeEdgeRow(key.first, key.second);
        }
    }

    lastSelectedItems.clear();
    
    ui->statusbar->showMessage(QString("Deleted %1 item(s)").arg(selectedItems.size()));
}

// zoom from the wheel event
void NetSim::handleZoom(QWheelEvent* event) {
    const double zoomFactor = 1.15;
    const double minZoom = 0.02;
    const double maxZoom = 10.0;

    double scaleFactor = 1.0;
    if (!event->angleDelta().isNull())
        scaleFactor = event->angleDelta().y() > 0 ? zoomFactor : 1.0 / zoomFactor;

    qreal currentZoom = ui->graphicsView->transform().m11();
    qreal newZoom = currentZoom * scaleFactor;

    // enforce zoom limits
    if (newZoom < minZoom || newZoom > maxZoom) {
        event->accept();
        return; 
    }

    ui->graphicsView->scale(scaleFactor, scaleFactor);
    event->accept();
}

// view Settings pop-up
void NetSim::onViewSettings() {
    QDialog dlg(this);
    dlg.setWindowTitle("View Settings");
    dlg.setMinimumWidth(280);

    auto* layout = new QVBoxLayout(&dlg);

    // edge label toggle
    auto* edgeLabelsCb = new QCheckBox("Show edge labels");
    edgeLabelsCb->setChecked(showEdgeLabels);
    edgeLabelsCb->setToolTip(
        "Toggle edge labels. Off deletes label objects to save resources.");

    // toggle gpu acceleration if available
    auto* gpuCb = new QCheckBox("Enable GPU acceleration (OpenGL)");
    bool currentlyUsingGPU = dynamic_cast<QOpenGLWidget*>(ui->graphicsView->viewport()) != nullptr;

    gpuCb->setChecked(currentlyUsingGPU);
    gpuCb->setToolTip("Uses OpenGL for rendering. Improves performance for large graphs. Lower quality farther away.");

    // toggle viewport update mode
    auto* viewportUpdateCb = new QCheckBox("Enable viewport update mode");
    viewportUpdateCb->setChecked(ui->graphicsView->viewportUpdateMode() == QGraphicsView::FullViewportUpdate);
    viewportUpdateCb->setToolTip(
        "Toggle between full redraw (slower) and smart partial updates (faster).");

    layout->addWidget(edgeLabelsCb);
    layout->addWidget(gpuCb);
    layout->addWidget(viewportUpdateCb);
    layout->addStretch(1);

    // apply and cancel buttons
    auto* btnBox = new QDialogButtonBox;
    auto* applyBtn = btnBox->addButton("Apply",  QDialogButtonBox::ApplyRole);
    auto* closeBtn = btnBox->addButton("Close",  QDialogButtonBox::RejectRole);
    layout->addWidget(btnBox);

    // apply the selection after apply button is hit
    connect(applyBtn, &QPushButton::clicked, this, [&]() {
        // update edge labels
        bool newShowEdgeLabels = edgeLabelsCb->isChecked();
        if (newShowEdgeLabels != showEdgeLabels) {
            showEdgeLabels = newShowEdgeLabels;

            for (NetworkEdge* e : edgeItems.values())
                e->setLabelVisible(showEdgeLabels);
        }

        // GPU toggle
        bool useGPU = gpuCb->isChecked();
        bool currentlyUsingGPU = dynamic_cast<QOpenGLWidget*>(ui->graphicsView->viewport()) != nullptr;

        if (useGPU != currentlyUsingGPU) {
            if (useGPU) {
                ui->graphicsView->setViewport(new QOpenGLWidget());
            } else {
                ui->graphicsView->setViewport(new QWidget());
            }

            ui->graphicsView->setScene(scene); 
            setupViewport();
            ui->graphicsView->setFocus();
            ui->graphicsView->viewport()->setFocus();
            ui->graphicsView->viewport()->update();
        }

        // toggle smart viewport update mode
        bool useFullViewport = viewportUpdateCb->isChecked();
        bool currentlyFullViewport = ui->graphicsView->viewportUpdateMode() == QGraphicsView::FullViewportUpdate;
        if (useFullViewport != currentlyFullViewport) {
            ui->graphicsView->setViewportUpdateMode(useFullViewport ? QGraphicsView::FullViewportUpdate : QGraphicsView::SmartViewportUpdate);
        }

        scene->update();
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();
}

// zoom in to the scene
void NetSim::onZoomIn() {
    const double maxZoom = 10.0;
    if (ui->graphicsView->transform().m11() < maxZoom)
        ui->graphicsView->scale(1.2, 1.2);
    ui->statusbar->showMessage("Zoomed in");
}

// zoom out of the scene
void NetSim::onZoomOut() {
    const double minZoom = 0.02;
    if (ui->graphicsView->transform().m11() > minZoom)
        ui->graphicsView->scale(1/1.2, 1/1.2);
    ui->statusbar->showMessage("Zoomed out");
}

// reset view be around all items in the scene
void NetSim::onResetView() {
    // with no nodes, just go back to center
    if (nodeItems.isEmpty()) {
        ui->graphicsView->resetTransform();
        ui->graphicsView->centerOn(0, 0);
        ui->statusbar->showMessage("View reset");
        return;
    }

    // Compute bounding rect from nodes only, ignoring border/edges
    QRectF nodeBounds;
    for (NetworkNode* node : nodeItems.values()) {
        QRectF r = node->mapToScene(node->boundingRect()).boundingRect();
        nodeBounds = nodeBounds.isNull() ? r : nodeBounds.united(r);
    }

    // padding
    nodeBounds.adjust(-100, -100, 100, 100); 
    ui->graphicsView->resetTransform();
    ui->graphicsView->fitInView(nodeBounds, Qt::KeepAspectRatio);
    ui->statusbar->showMessage("View reset");
}

// when the selected item moves, node or edge
void NetSim::onSelectionChanged() {
    // Get currently selected items
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();

    QString lastName = "None";
    
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
        // if its a node and not selected, reset it and all its edges
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            item->setZValue(NetworkNode::DEFAULT_ZVALUE);

            // Get incident edges from backend
            const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(node->nodeId);

            // for each edges, reset the z value of the visual edge
            for (const EdgeInfo& e : incidentEdges) {
                NetworkEdge* edge = edgeItems.value(qMakePair(node->nodeId, e.destination));
                if (!edge && !directedEdges) {
                    edge = edgeItems.value(qMakePair(e.destination, node->nodeId));
                }
                if (edge) edge->setZValue(NetworkEdge::DEFAULT_ZVALUE);
            }
        } 
        // if its a edge and not selected, reset it
        else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            item->setZValue(NetworkEdge::DEFAULT_ZVALUE);
        }
    }

    
    // If nothing is selected, we're done
    if (selectedItems.isEmpty()) { 
        return; 
    }
    
    // Set all newly selected items to selected z value
    for (QGraphicsItem* item : selectedItems) {
        // if we select a node, set it and all its edges to selected
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            item->setZValue(NetworkNode::SELECTED_ZVALUE);
            const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(node->nodeId);

            // for all its incident edges, set the z value of the visual edge to selected
            for (const EdgeInfo& e : incidentEdges) {
                NetworkEdge* edge = edgeItems.value(qMakePair(node->nodeId, e.destination));
                if (!edge && !directedEdges) {
                    edge = edgeItems.value(qMakePair(e.destination, node->nodeId));
                }
                if (edge) edge->setZValue(NetworkEdge::SELECTED_ZVALUE);
            }
        } 
        // if we select an edge
        else if (NetworkEdge* edge = dynamic_cast<NetworkEdge*>(item)) {
            item->setZValue(NetworkEdge::SELECTED_ZVALUE);
        }
    }
    
    // Update last selected item
    lastSelectedItems = selectedItems;
}

// ------------------------------
// Load graph from edge list file
// ------------------------------
void NetSim::onLoadGraph()
{
    // load the file
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load Graph",
        QDir::homePath(),
        "Edge List Files (*.txt *.csv *.edges *.el *.tsv);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    // open the files
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Load Graph", "Could not open file:\n" + fileName);
        return;
    }

    // clear current graph
    clearGraph();    

    // temporarily block signals
    scene->blockSignals(true);

    QTextStream in(&file);
    struct EdgeEntry { QString src, dst, label; };
    QList<EdgeEntry> edgeEntries;
    QSet<QString> uniqueNodeLabels;

    int lineNum = 0;
    int skipCount = 0;

    // parse the file
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNum++;

        // some basic line skipping 
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('%') || line.startsWith("//"))
            continue;

        QStringList parts = line.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            skipCount++;
            continue;
        }

        // add temp edge entry and track unique node labels
        QString src = parts[0];
        QString dst = parts[1];
        QString weight = (parts.size() >= 3) ? parts[2] : "";

        edgeEntries.append({src, dst, weight});
        uniqueNodeLabels.insert(src);
        uniqueNodeLabels.insert(dst);
    }
    file.close();

    // build unique node for the backend
    QMap<QString, int> labelToId;
    for (const QString& label : uniqueNodeLabels) {
        int id = dataHandler->addNode(label);
        labelToId[label] = id;
    }

    // Build edges for the backend
    for (const EdgeEntry& e : edgeEntries) {
        int srcId = labelToId[e.src];
        int dstId = labelToId[e.dst];
        dataHandler->addEdge(srcId, dstId, e.label);
        if (!directedEdges) {
            dataHandler->addEdge(dstId, srcId, e.label);
        }
    }

    // build the visual nodes and keep track of them for edge creation
    QMap<int, NetworkNode*> idToVisual;
    for (auto it = labelToId.begin(); it != labelToId.end(); ++it) {
        const QString& label = it.key();
        int nodeId = it.value();

        NetworkNode* node = new NetworkNode(0, 0, label);
        node->nodeId = nodeId;

        scene->addItem(node);
        idToVisual[nodeId] = node;
        nodeItems[nodeId] = node;
    }

    // create the edge Items
    for (const EdgeEntry& e : edgeEntries) {
        int srcId = labelToId[e.src];
        int dstId = labelToId[e.dst];

        // Skip duplicate edges if multiEdges is false
        if (!multiEdges) {
            if (edgeItems.contains(qMakePair(srcId, dstId)) || (!directedEdges && edgeItems.contains(qMakePair(dstId, srcId)))) {
                continue;
            }
        }

        // make the edge item
        NetworkNode* srcNode = idToVisual[srcId];
        NetworkNode* dstNode = idToVisual[dstId];
        NetworkEdge* edge = new NetworkEdge(srcNode, dstNode, directedEdges, e.label, nullptr, showEdgeLabels);
        scene->addItem(edge);

        // store the edge
        edgeItems.insert(qMakePair(srcId, dstId), edge);
        if (!directedEdges) {
            edgeItems.insert(qMakePair(dstId, srcId), edge);
        }
    }

    // Unblock and do one single update pass
    scene->blockSignals(false);
    updateEdges();
    updateSceneRect();

    // default layout to a spiral
    algorithmPanel->runSpiralLayout(false);

    onResetView();

    QString msg = QString("Loaded %1 nodes, %2 edges from \"%3\"")
                      .arg(nodeItems.size())
                      .arg(edgeItems.size())
                      .arg(QFileInfo(fileName).fileName());
    if (skipCount > 0)
        msg += QString("  (%1 malformed line(s) skipped)").arg(skipCount);

    ui->statusbar->showMessage(msg);
}


// ----------------------------------
// other netsim functions
// ----------------------------------

// Update all edges when nodes move
void NetSim::updateEdges() {
    if (updatingEdges) return;
    updatingEdges = true;

    for (NetworkEdge* edge : edgeItems) {
        if (edge) {
            edge->updatePosition();
        }
    }
    updateSceneRect();

    if (graphPanel) graphPanel->updateNodePositions();

    updatingEdges = false;
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
    AddEdge(node1, node2, directedEdges, "-5", false);
    AddEdge(node2, node3, directedEdges, "2", false);
    AddEdge(node2, node4, directedEdges, "3", false);
    AddEdge(node1, node4, directedEdges, "45", false);
    AddEdge(node4, node5, directedEdges, "-47", false);
    AddEdge(node4, node3, directedEdges, "39", false);

    
    ui->statusbar->showMessage("Sample network created with 5 nodes and 4 edges");
}