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
    setBrush(QBrush(Qt::lightGray));
    setPen(QPen(Qt::darkBlue, 2));
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
    
    // Draw bright blue border when selected (for normal nodes)
    if (option->state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor(30, 144, 255), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(boundingRect().adjusted(1, 1, -1, -1));
    }

    // Draw label with truncation for non‑contracted nodes
    painter->setPen(Qt::darkBlue);
    qreal availableWidth = boundingRect().width() - 10;
    QFontMetrics fm(painter->font());
    QString displayLabel = fm.elidedText(getLabel(), Qt::ElideRight, availableWidth);
    painter->drawText(boundingRect(), Qt::AlignCenter, displayLabel);

    painter->setRenderHint(QPainter::Antialiasing);

    if (m_contracted) {
        const bool selected = option->state & QStyle::State_Selected;
        const qreal r = m_contractedRadius;

        //  light blue fill, default pen (same as normal node)
        painter->setBrush("#a0cbff"); 
        painter->setPen(pen());
        painter->drawEllipse(QRectF(-r, -r, r * 2, r * 2));

        // bright blue border
        if (selected) {
            painter->setPen(QPen(QColor(30, 144, 255), 3));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QRectF(-r, -r, r * 2, r * 2).adjusted(1, 1, -1, -1));
        }

        // "x{N}" label
        painter->setPen(Qt::darkBlue);
        QFont f;
        f.setPointSize(qBound(7, (int)(r * 0.45), 18));
        painter->setFont(f);
        painter->drawText(QRectF(-r, -r, r * 2, r * 2),
                          Qt::AlignCenter,
                          QString("x%1").arg(m_memberFrontIds.size()));
        return;
    }
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
        // only update edges connected to this node, not the whole scene
        for (NetworkEdge* edge : m_edges)
            edge->updatePosition();
    }

    return QGraphicsEllipseItem::itemChange(change, value);
}

// set the label of a nodes item and update the display
void NetworkNode::setLabel(const QString& label) {
    fullLabelText = label;
    update();
}

// make this node contracted
void NetworkNode::setContracted(const QVector<int>& memberFrontIds) {
    m_contracted = true;
    m_memberFrontIds = memberFrontIds;
    m_contractedRadius = qMin(BASE_RADIUS + RADIUS_PER_NODE * memberFrontIds.size(), MAX_RADIUS);

    const qreal r = m_contractedRadius;
    setRect(-r, -r, r * 2, r * 2);
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

    if (srcNode) srcNode->registerEdge(this);
    if (dstNode) dstNode->registerEdge(this);

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

    // selected edges 
    if (isSelected()) {
        // purple if contracted, bright blue if not
        if(m_contractedEdge){
            const qreal minThickness = 3.0;
            const qreal maxThickness = 15.0;
            qreal t = qMin((qreal)m_contractedCount / m_totalNodes, 1.0);
            qreal thickness = minThickness + t * (maxThickness - minThickness);

            if(m_contractedCount == 1) {
                thickness = 3.0;
            }

            painter->setPen(QPen(QColor(30, 144, 255), thickness, Qt::SolidLine, Qt::RoundCap));
        }
        else
            painter->setPen(QPen(QColor(30, 144, 255), 3, Qt::SolidLine, Qt::RoundCap));
    } 
    else if (m_contractedEdge) {
        
        const qreal minThickness = 3.0;
        const qreal maxThickness = 15.0;
        qreal t = qMin((qreal)m_contractedCount / m_totalNodes, 1.0);
        qreal thickness = minThickness + t * (maxThickness - minThickness);

        if(m_contractedCount == 1) {
            thickness = 3.0;
        }

        QPen pen(QColor(140, 60, 200), thickness, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(pen);
    }
    else {
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
    if (!srcNode || !dstNode) return;
    QLineF newLine(srcNode->pos(), dstNode->pos());
    if (line() == newLine) return;
    setLine(newLine);
    updateLabelPosition();
}

// set an edge to be contracted
void NetworkEdge::setContracted(bool contracted, int count, int totalNodes) {
    m_contractedEdge = contracted;
    m_contractedCount = count;
    m_totalNodes = totalNodes;
    update();
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
    // scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
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
    pw.nodeItemsLbl = ui->nodeItemsLabel;
    pw.edgeItemsLbl = ui->edgeItemsLabel;
    pw.titleLbl     = ui->panelTitleLabel;
    pw.splitter     = ui->mainSplitter;

    graphPanel = new GraphPanel(this, pw, this);
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

    connect(graphPanel, &GraphPanel::contractRequested, this, &NetSim::onContractSelected);

    connect(graphPanel, &GraphPanel::deleteRequested, this, &NetSim::onDeleteSelected);

    connect(graphPanel, &GraphPanel::findRequested, this, [this]() {
        // Collect the bounding rect of all selected scene items and fit the view to it
        QList<QGraphicsItem*> selected = scene->selectedItems();
        if (selected.isEmpty()) return;

        QRectF bounds;
        for (QGraphicsItem* item : selected) {
            QRectF r = item->mapToScene(item->boundingRect()).boundingRect();
            bounds = bounds.isNull() ? r : bounds.united(r);
        }

        // add padding
        bounds.adjust(-80, -80, 80, 80);
        ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
        ui->statusbar->showMessage("Found selection in graph.");
    });

    // conect expanding requested
    connect(graphPanel, &GraphPanel::expandRequested, this, [this](int nodeId) {
        NetworkNode* node = nodeItems.value(nodeId);
        if (node) {
            onExpandNode(node);
        }
    });

    // connect move to origin requested
    connect(graphPanel, &GraphPanel::moveToOriginRequested, this, [this]() {
        for (NetworkNode* node : nodeItems) {
            if (node->isSelected())
                node->setPos(0, 0);
        }
        updateEdges();
        ui->statusbar->showMessage("Moved selected node(s) to origin.");
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
    algorithmPanel = new AlgorithmPanel(this, ui->algoPanelContainer, scene, sceneBorder);
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

    // remove nodes and edges
    QSet<NetworkEdge*> uniqueEdges(edgeItems.begin(), edgeItems.end());
    for (NetworkEdge* edge : uniqueEdges) delete edge;
    edgeItems.clear();

    for (NetworkNode* node : nodeItems) delete node;
    nodeItems.clear();
    
    // Clear remaining items
    scene->clear();
    


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

    if (scene->sceneRect() != finalRect)
        scene->setSceneRect(finalRect);

    // readjust border
    if (sceneBorder) {
        QRectF borderRect = finalRect.adjusted(3, 3, -3, -3);
        if (sceneBorder->rect() != borderRect)
            sceneBorder->setRect(borderRect);
    }
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
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // full redraws
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

    // if multiple normal nodes are selected, show contract option
    QList<QGraphicsItem*> selected = scene->selectedItems();
    QList<NetworkNode*> selectedNodes;

    // get selected nodes
    for (QGraphicsItem* item : selected) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            selectedNodes.append(node);
        }
    }

    // add contract selected
    if (selectedNodes.size() >= 2) {
        QAction* contractAction = menu.addAction("Contract selected");
        connect(contractAction, &QAction::triggered, this, &NetSim::onContractSelected);
    }

    // add position to 0 option
    if(selectedNodes.size() >= 1) {        
        QAction* moveToOriginAction = menu.addAction("Move selected to origin");
        connect(moveToOriginAction, &QAction::triggered, this, [this, selectedNodes]() {
            for (NetworkNode* node : selectedNodes)
                node->setPos(0, 0);
            updateEdges();
            ui->statusbar->showMessage(QString("Moved %1 node(s) to origin.").arg(selectedNodes.size()));
        });
    }

    
    // if on a node show node menu
    if (clickedNode) {
        // node that is clicked on
        NetworkNode* targetNode = clickedNode;
        
        // Node context menu actions, add edge or delete node
        if(!clickedNode->isContracted()) {


            QAction* addLabel = menu.addAction("Edit label");
            QAction* addEdgeAction = menu.addAction("Add edge");

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

            // connect add label action
            connect(addLabel, &QAction::triggered, this, [this, targetNode]() {
                onEditNodeLabel(targetNode);
            });
        }

        if (clickedNode->isContracted()) {
            QAction* expandAction = menu.addAction("Expand");
            connect(expandAction, &QAction::triggered, this, [this, clickedNode]() {
                onExpandNode(clickedNode);
            });
        }

        QAction* deleteAction = menu.addAction("Delete");
        
        // connect delete action
        connect(deleteAction, &QAction::triggered, this, [this, targetNode]() {
            onDeleteSelected();
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

// contraction methods

int NetSim::registerContractedNode(NetworkNode* contracted, const QVector<int>& memberFrontIds)
{
    int id = m_nextContractedId--;
    contracted->nodeFrontId = id;
    contracted->setContracted(memberFrontIds);
    m_contractedMembers[id] = memberFrontIds;
    return id;
}

void NetSim::setNodeContractedMapping(int backendNodeId, int nodeFrontId) {
    m_backIdToFrontId[backendNodeId] = nodeFrontId;
}

// expand a contracted node
void NetSim::onExpandNode(NetworkNode* contractedNode) {
    if (!contractedNode || !contractedNode->isContracted()) return;

    int contractedId = contractedNode->nodeFrontId;
    QVector<int> memberBackIds = m_contractedMembers.value(contractedId);
    if (memberBackIds.isEmpty()) return;

    for (int backId : memberBackIds) {
        m_backIdToFrontId[backId] = backId;
    }

    QSet<NetworkEdge*> edgesToDelete;

    // delete edge connected to the contracted node
    for (auto it = edgeItems.begin(); it != edgeItems.end(); ) {
        int fSrc = it.key().first;
        int fDst = it.key().second;

        if (fSrc == contractedId || fDst == contractedId) {
            edgesToDelete.insert(it.value());
            it = edgeItems.erase(it);
        } else {
            ++it;
        }
    }

    for (NetworkEdge* edge : edgesToDelete) {
        scene->removeItem(edge);
        delete edge;
    }
    

    // Remove the contracted node from front‑end
    nodeItems.remove(contractedId);
    scene->removeItem(contractedNode);

    // Recreate all member nodes using backend data
    QHash<int, NetworkNode*> newNodes;
    QPointF center = contractedNode->pos();
    int count = memberBackIds.size();
    for (int i = 0; i < count; ++i) {
        int nodeId = memberBackIds[i];
        const NodeInfo* info = dataHandler->getNode(nodeId);
        if (!info) continue;

        // Place nodes in a small circle around the contracted node's position
        qreal angle = 2.0 * M_PI * i / count;
        qreal radius = 40.0 + count * 2.0; 
        QPointF pos = center + QPointF(radius * cos(angle), radius * sin(angle));

        NetworkNode* node = new NetworkNode(pos.x(), pos.y(), dataHandler->nodeLabel(nodeId));
        node->nodeFrontId = nodeId;
        scene->addItem(node);

        if (nodeItems.contains(nodeId)) {
            delete nodeItems[nodeId];
        }
        nodeItems[nodeId] = node;
        newNodes[nodeId] = node;
    }

    // Restore edges among member nodes
    for (int src : memberBackIds) {
        const QVector<EdgeInfo> edges = dataHandler->getEdgesOf(src);
        for (const EdgeInfo& e : edges) {
            int dst = e.destination;

            int fSrc = backIdToFrontId(src);
            int fDst = backIdToFrontId(dst);

            AddVisualEdge(fSrc, fDst, e.label, directedEdges);
        }
    }

    // Clean up mapping
    m_contractedMembers.remove(contractedId);

    delete contractedNode;

    // Update panels and scene
    updateSceneRect();
    if (graphPanel) graphPanel->refresh();
    ui->statusbar->showMessage(QString("Expanded component with %1 nodes").arg(memberBackIds.size()));
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
                    int srcId = edgeSourceNode->nodeFrontId;
                    int dstId = destNode->nodeFrontId;
                    bool edgeExists = dataHandler->edgeExists(srcId, dstId);
                    if (!directedEdges && !edgeExists) {
                        edgeExists = dataHandler->edgeExists(dstId, srcId);
                    }

                    // dont do edges to contracted nodes
                    if ((destNode->isContracted() || edgeSourceNode->isContracted())) {
                        QMessageBox::warning(this, "Invalid edge", 
                            "Use the \"Add Edge\" button to create edges to or from contracted nodes.");
                        cleanupEdgeCreation();
                        return true;
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

// contract all selected nodes into 1
void NetSim::onContractSelected() {
    // Collect all selected nodes 
    QList<NetworkNode*> selectedNodes;
    for (QGraphicsItem* item : scene->selectedItems()) {
        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item))
            selectedNodes.append(node);
    }

    if (selectedNodes.size() < 2) {
        QMessageBox::information(this, "Contract", "Select at least two nodes to contract.");
        return;
    }

    // Gather all backend IDs from the selected nodes
    QSet<int> finalMembers;
    QSet<int> frontIdsBeingRemoved;
    QList<int> frontIdsToRemove;

    QPointF centroid(0, 0);
    int posCount = 0;

    for (NetworkNode* node : selectedNodes) {
        centroid += node->pos();
        ++posCount;

        int frontId = node->nodeFrontId;
        frontIdsBeingRemoved.insert(frontId);

        if (node->isContracted()) {
            const QVector<int>& members = m_contractedMembers.value(frontId);
            for (int id : members)
                finalMembers.insert(id);

            frontIdsToRemove.append(frontId);
        } else {
            finalMembers.insert(frontId);
        }
    }

    QVector<int> allBackIds = QVector<int>(finalMembers.begin(), finalMembers.end());

    if (allBackIds.size() < 2) {
        QMessageBox::information(this, "Contract", "The selected nodes contain fewer than two distinct backend nodes.");
        return;
    }

    centroid /= posCount;

    scene->blockSignals(true);

    // Create the new contracted node
    NetworkNode* contracted = new NetworkNode(
        centroid.x(), centroid.y(),
        QString("Contracted (%1)").arg(allBackIds.size())
    );
    int newFrontId = registerContractedNode(contracted, allBackIds);
    contracted->nodeFrontId = newFrontId;
    scene->addItem(contracted);
    nodeItems[newFrontId] = contracted;

    // Update front mapping for all involved backend nodes
    for (int backId : allBackIds) {
        m_backIdToFrontId[backId] = newFrontId;
    }

    // Remove old contracted mappings
    for (int oldFrontId : frontIdsToRemove) {
        m_contractedMembers.remove(oldFrontId);
    }

    // get edges to be removed
    QSet<QPair<int,int>> removedEdgeKeys;
    for (auto it = edgeItems.begin(); it != edgeItems.end(); ++it) {
        int u = it.key().first;
        int v = it.key().second;

        if (frontIdsBeingRemoved.contains(u) || frontIdsBeingRemoved.contains(v)) {
            removedEdgeKeys.insert({qMin(u, v), qMax(u, v)});
        }
    }

    // Collect edges to delete
    QSet<NetworkEdge*> edgesToDelete;
    for (auto it = edgeItems.begin(); it != edgeItems.end(); ++it) {
        const QPair<int,int>& key = it.key();
        if (frontIdsBeingRemoved.contains(key.first) || frontIdsBeingRemoved.contains(key.second)) {
            edgesToDelete.insert(it.value());
        }
    }

    // Remove from map
    for (auto it = edgeItems.begin(); it != edgeItems.end(); ) {
        if (edgesToDelete.contains(it.value())) {
            it = edgeItems.erase(it);
        } else {
            ++it;
        }
    }

    // Delete edges
    for (NetworkEdge* edge : edgesToDelete) {
        scene->removeItem(edge);
        delete edge;
    }

    
    // Remove the original selected nodes
    for (NetworkNode* node : selectedNodes) {
        nodeItems.remove(node->nodeFrontId);
        scene->removeItem(node);
        delete node;
    }

    // Build external connections
    QHash<int, int>     externalConnectors;   // otherFrontId → total edge count
    QHash<int, QString> externalEdgeLabel;
    QSet<QPair<int,int>> visitedBack;

    for (int backId : allBackIds) {
        for (const EdgeInfo& e : dataHandler->getEdgesOf(backId)) {
            int otherBackId = e.destination;
            if (finalMembers.contains(otherBackId)) continue;

            QPair<int,int> backKey = directedEdges
                ? qMakePair(backId, otherBackId)
                : qMakePair(qMin(backId, otherBackId), qMax(backId, otherBackId));
            if (visitedBack.contains(backKey)) continue;
            visitedBack.insert(backKey);

            int otherFrontId = m_backIdToFrontId.value(otherBackId, otherBackId);
            externalConnectors[otherFrontId]++;
            if (!externalEdgeLabel.contains(otherFrontId))
                externalEdgeLabel[otherFrontId] = e.label;
        }
    }

    const int totalNodes = nodeItems.size();
    for (auto it = externalConnectors.cbegin(); it != externalConnectors.cend(); ++it) {
        int externalFrontId = it.key();
        int count           = it.value();
        NetworkNode* externalNode = nodeItems.value(externalFrontId);
        if (!externalNode) continue;

        NetworkEdge* mergedEdge = new NetworkEdge(
            contracted, externalNode, directedEdges,
            QString("x%1").arg(count), nullptr, showEdgeLabels);
        mergedEdge->setContracted(true, count, totalNodes);
        scene->addItem(mergedEdge);
        edgeItems[qMakePair(newFrontId, externalFrontId)] = mergedEdge;
        if (!directedEdges)
            edgeItems[qMakePair(externalFrontId, newFrontId)] = mergedEdge;
    }

    // Update scene
    scene->blockSignals(false);
    scene->update();
    updateSceneRect();

    // Update graph panel
    if (graphPanel) {
        // Remove nodes
        for (int frontId : frontIdsBeingRemoved) {
            graphPanel->removeNodeRow(frontId);
        }

        for (const QPair<int,int>& key : removedEdgeKeys)
            graphPanel->removeEdgeRow(key.first, key.second);

        // Add new node + edges
        graphPanel->addNodeRow(newFrontId);
        for (auto it = externalConnectors.begin(); it != externalConnectors.end(); ++it) {
            int externalFrontId = it.key();
            if (!nodeItems.contains(externalFrontId)) continue;
            graphPanel->addEdgeRow(newFrontId, externalFrontId);
        }
    }

    ui->statusbar->showMessage(
        QString("Contracted %1 nodes into one").arg(allBackIds.size())
    );
}

// clear the entire graph
void NetSim::clearGraph() {
    cleanupEdgeCreation();
    lastSelectedItems.clear();
    scene->clearSelection();

    // deduplicate edges since undirected edges are stored under 2 keys pointing to the same pointer
    QSet<NetworkEdge*> uniqueEdges(edgeItems.begin(), edgeItems.end());
    for (NetworkEdge* edge : uniqueEdges)
        delete edge;
    edgeItems.clear();

    // delete nodes safely
    for (NetworkNode* node : nodeItems)
        delete node;
    nodeItems.clear();

    // clear the datahandler data
    dataHandler->clear();

    // clear remaining scene items 
    scene->clear();
    sceneBorder = nullptr;

    // re-create the border
    sceneBorder = new QGraphicsRectItem();
    sceneBorder->setPen(QPen(QColor(180, 180, 180), 10));
    sceneBorder->setBrush(Qt::NoBrush);
    sceneBorder->setZValue(-1);
    scene->addItem(sceneBorder);
    algorithmPanel->setSceneBorder(sceneBorder);

    if (graphPanel) graphPanel->clear();

    m_backIdToFrontId.clear();
    m_contractedMembers.clear();
    m_nextContractedId = -1;

    updateSceneRect();
}

// clear just the front end items
void NetSim::resetFrontendState() {
    m_backIdToFrontId.clear();
    m_contractedMembers.clear();
    m_nextContractedId = -1;
}

// add node action
void NetSim::onAddNode() {
    bool ok;
    QString label = QInputDialog::getText(this, "Add Node", "Node Label:", QLineEdit::Normal, 
        QString("Node%1").arg(dataHandler->nextNodeLabel() + 1), &ok);
    
    // Add at center of view 
    if (ok && !label.isEmpty()) {
        QPoint viewCenter = ui->graphicsView->viewport()->rect().center();
        QPointF scenePos = ui->graphicsView->mapToScene(viewCenter);
        AddNodeAt(scenePos, label);
    }
}

// add node at specific position
NetworkNode* NetSim::AddNodeAt(const QPointF& position, const QString& label, int initialCapacity, int nodeId) {
    if (nodeId > -1) {
        // Node already exists in backend, use its label
        const NodeInfo* info = dataHandler->getNode(nodeId);
        if (!info) {
            qWarning() << "AddNodeAt: Node ID" << nodeId << "not found in backend";
            return nullptr;
        }

        // get the label and make the frontend node
        QString label = dataHandler->nodeLabel(nodeId);
        NetworkNode* node = new NetworkNode(position.x(), position.y(), label);

        node->nodeFrontId = nodeId;
        scene->addItem(node);
        nodeItems[nodeId] = node;

        if (graphPanel) graphPanel->addNodeRow(nodeId);
        updateSceneRect();
        return node;
    } 
    else {
        // new node gets added to database
        nodeId = dataHandler->addNode(label, initialCapacity);

        // then create visual node
        NetworkNode* node = new NetworkNode(position.x(), position.y(), label);

        node->nodeFrontId = nodeId;
        scene->addItem(node);
        nodeItems[nodeId] = node;

        if (graphPanel) graphPanel->addNodeRow(nodeId);
        updateSceneRect();
        return node;
    }
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
        if (!n->isContracted()) sourceCbo->addItem(n->getLabel(), QVariant::fromValue(static_cast<void*>(n)));
    form->addRow("Source:", sourceCbo);

    // Destination combo
    auto* destCbo = new QComboBox;
    for (NetworkNode* n : nodeItems.values())
        if (!n->isContracted()) destCbo->addItem(n->getLabel(), QVariant::fromValue(static_cast<void*>(n)));
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
        int srcId = src->nodeFrontId;
        int dstId = dest->nodeFrontId;
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
        int nodeFrontId = targetNode->nodeFrontId;
        dataHandler->setNodeLabel(nodeFrontId, newLabel);
        nodeItems.value(nodeFrontId)->setLabel(newLabel);
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
        dataHandler->setEdgeLabel(clickedEdge->sourceNode()->nodeFrontId, clickedEdge->destNode()->nodeFrontId, newLabel);
        ui->statusbar->showMessage(QString("Edge label updated to: %1").arg(newLabel));
    }
}

// add edge to scene given the two, directed or not, and label
void NetSim::AddEdge(NetworkNode* sourceNode, NetworkNode* destNode, bool directed, const QString& label, bool editLabel) {
    if (!sourceNode || !destNode) return;

    int srcId = sourceNode->nodeFrontId;
    int dstId = destNode->nodeFrontId;

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
    if (graphPanel) {
        graphPanel->updateNodeRow(srcId);
        graphPanel->updateNodeRow(dstId);
        graphPanel->addEdgeRow(srcId, dstId);
    }
}

void NetSim::AddVisualEdge(int srcId, int dstId, const QString& label, bool directed){
    if (!nodeItems.contains(srcId) || !nodeItems.contains(dstId)) return;
    if (srcId == dstId) return;

    QPair<int,int> key = directed
        ? qMakePair(srcId, dstId)
        : qMakePair(qMin(srcId, dstId), qMax(srcId, dstId));

    if (edgeItems.contains(key)) return;

    NetworkNode* srcNode = nodeItems.value(srcId);
    NetworkNode* dstNode = nodeItems.value(dstId);

    NetworkEdge* edge = new NetworkEdge(srcNode, dstNode, directed, label, nullptr, showEdgeLabels);
    scene->addItem(edge);

    edgeItems[key] = edge;
    if (!directed)
        edgeItems[qMakePair(key.second, key.first)] = edge;


    // update table
    if (graphPanel) {
        graphPanel->addEdgeRow(srcId, dstId);
        graphPanel->updateNodeRow(srcId);
        graphPanel->updateNodeRow(dstId);
    }
}


// delete an edge from the scene and both nodes
void NetSim::deleteEdge(NetworkEdge* edge) {
    if (!edge) return;

    // get the edge nodes ids
    NetworkNode* src = edge->sourceNode();
    NetworkNode* dst = edge->destNode();
    int srcFrontId = src->nodeFrontId;
    int dstFrontId = dst->nodeFrontId;

    // for contracted edges
    if (edge->isContractedEdge()) {
        // Identify which endpoint is the contracted node
        int contractedFrontId = (srcFrontId < 0) ? srcFrontId : dstFrontId;
        int externalFrontId   = (srcFrontId < 0) ? dstFrontId : srcFrontId;

        // Resolve the external side to a set of backend IDs
        QSet<int> externalBackIds;
        if (externalFrontId >= 0) {
            externalBackIds.insert(externalFrontId);
        } else {
            for (int id : m_contractedMembers.value(externalFrontId))
                externalBackIds.insert(id);
        }

        // Delete every backend edge from a contracted member to the external side
        for (int backId : m_contractedMembers.value(contractedFrontId)) {
            for (const EdgeInfo& e : dataHandler->getEdgesOf(backId)) {
                int destFrontId = m_backIdToFrontId.value(e.destination, e.destination);
                if (destFrontId == externalFrontId || externalBackIds.contains(e.destination)) {
                    dataHandler->removeEdge(backId, e.destination);
                    if (!directedEdges)
                        dataHandler->removeEdge(e.destination, backId);
                }
            }
        }
    } 
    else {
        // Normal edge is direct backend removal
        dataHandler->removeEdge(srcFrontId, dstFrontId);
        if (!directedEdges)
            dataHandler->removeEdge(dstFrontId, srcFrontId);
    }

    // Remove from map
    edgeItems.remove(QPair<int,int>(srcFrontId, dstFrontId));
    if (!directedEdges) {
        edgeItems.remove(QPair<int,int>(dstFrontId, srcFrontId));
    }

    lastSelectedItems.removeOne(edge);

    scene->removeItem(edge);
    delete edge;
}

// delete node from the scene and all connected edges
void NetSim::deleteNode(NetworkNode* node) {
    if (!node) return;
    int nodeFrontId = node->nodeFrontId;

    lastSelectedItems.removeOne(node);

    // if node is contracted
    if (node->isContracted()) {
        QVector<int> memberBackIds = m_contractedMembers.value(nodeFrontId);

        // Delete all backend edges incident to each member
        for (int backId : memberBackIds) {
            const QVector<EdgeInfo> incident = dataHandler->getEdgesOf(backId);
            for (const EdgeInfo& e : incident) {
                dataHandler->removeEdge(backId, e.destination);
                if (!directedEdges)
                    dataHandler->removeEdge(e.destination, backId);
            }
        }

        // Delete all member nodes from backend
        for (int backId : memberBackIds) {
            dataHandler->removeNodeNoEdges(backId);
            m_backIdToFrontId.remove(backId);
            if (graphPanel)
                graphPanel->removeNodeRow(backId);
        }

        QSet<NetworkEdge*> edgesToDelete;
        QSet<QPair<int,int>> normKeysForPanel;

        // get edges to delete
        for (auto it = edgeItems.begin(); it != edgeItems.end(); ) {
            QPair<int,int> key = it.key();
            if (key.first == nodeFrontId || key.second == nodeFrontId) {
                edgesToDelete.insert(it.value());
                normKeysForPanel.insert({qMin(key.first, key.second), qMax(key.first, key.second)});
                it = edgeItems.erase(it);
            } else {
                ++it;
            }
        }

        // Remove panel rows before deleting pointers
        if (graphPanel) {
            for (const QPair<int,int>& key : normKeysForPanel)
                graphPanel->removeEdgeRow(key.first, key.second);
        }

        // delete edge items
        for (NetworkEdge* edge : edgesToDelete) {
            scene->removeItem(edge);
            delete edge;
        }

        // delete node item
        m_contractedMembers.remove(nodeFrontId);
        nodeItems.remove(nodeFrontId);
        scene->removeItem(node);
        delete node;

        if (graphPanel) graphPanel->removeNodeRow(nodeFrontId);
        return;
    }

    // Get all edges incident to this node from backend
    const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(nodeFrontId);
    for (const EdgeInfo& e : incidentEdges) {
        int neighborFrontId = e.destination;

        // Look up the visual edge
        NetworkEdge* edge = edgeItems.value(QPair<int,int>(nodeFrontId, neighborFrontId));
        if (!edge && !directedEdges) {
            edge = edgeItems.value(QPair<int,int>(neighborFrontId, nodeFrontId));
        }
        if (edge) {
            deleteEdge(edge);
        }
    }

    // removes a node with no edges so we dont have to loop through edges
    dataHandler->removeNodeNoEdges(nodeFrontId);

    // remove item node
    nodeItems.remove(nodeFrontId);
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
        QPair<int,int> key = {qMin(edge->sourceNode()->nodeFrontId, edge->destNode()->nodeFrontId),
                              qMax(edge->sourceNode()->nodeFrontId, edge->destNode()->nodeFrontId)};
        edgeKeysToDelete.append(key);
    }

    // First delete nodes and their incident edges
    for (NetworkNode* node : selectedNodes) {
        int nodeFrontId = node->nodeFrontId;

        // capture incident edge keys BEFORE deleteNode removes them
        const QVector<EdgeInfo> incident = dataHandler->getEdgesOf(nodeFrontId);
        QList<QPair<int,int>> incidentKeys;
        for (const EdgeInfo& e : incident)
            incidentKeys.append({qMin(nodeFrontId, e.destination), qMax(nodeFrontId, e.destination)});

        deleteNode(node);

        // remove node row and all incident edge rows from the panel
        if (graphPanel) {
            graphPanel->removeNodeRow(nodeFrontId);
            for (const QPair<int,int>& key : incidentKeys)
                graphPanel->removeEdgeRow(key.first, key.second);
        }
    }

    // Second delete any explicitly selected edges not already deleted by deleteNode
    for (const QPair<int,int>& key : edgeKeysToDelete) {
        if (edgeItems.contains(key)) {
            deleteEdge(edgeItems.value(key));
            if (graphPanel) {
                graphPanel->removeEdgeRow(key.first, key.second);
                graphPanel->updateNodeRow(key.first);
                graphPanel->updateNodeRow(key.second);
            }
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


    // add default layout selection
    auto* layoutGroupBox = new QGroupBox("Default layout on graph load");
    auto* lgLayout = new QVBoxLayout(layoutGroupBox);

    auto* algoCombo = new QComboBox;
    algoCombo->addItem("None", "none");
    algoCombo->addItem("Circular", "circular");
    algoCombo->addItem("Spiral", "spiral");
    algoCombo->addItem("SFDP", "sfdp");
    algoCombo->addItem("Contract Components", "compContract");
    algoCombo->addItem("Contract High-Degrees", "ContractHighDegrees");

    // pre-select the current default
    int comboIdx = algoCombo->findData(m_defaultLayoutAlgo);
    if (comboIdx != -1) algoCombo->setCurrentIndex(comboIdx);

    auto* configureBtn = new QPushButton("Configure parameters…");
    configureBtn->setEnabled(m_defaultLayoutAlgo != "none");

    // enable/disable configure button as selection changes
    connect(algoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [&](int) {
        QString algo = algoCombo->currentData().toString();
        configureBtn->setEnabled(algo != "none" && algo != "compContract");
    });

    // open the algorithm's own param dialog
    connect(configureBtn, &QPushButton::clicked, this, [&]() {
        algorithmPanel->configureLayoutParams(algoCombo->currentData().toString());
    });

    lgLayout->addWidget(algoCombo);
    lgLayout->addWidget(configureBtn);

    layout->addWidget(layoutGroupBox);


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

        m_defaultLayoutAlgo = algoCombo->currentData().toString();

        scene->update();
        dlg.accept(); 
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
        // Skip if the item is no longer in the scene (deleted)
        if (!scene->items().contains(item))
            continue;

        if (NetworkNode* node = dynamic_cast<NetworkNode*>(item)) {
            // For contracted nodes (negative ID), skip edge reset – they have no backend edges
            if (node->nodeFrontId < 0) {
                item->setZValue(NetworkNode::DEFAULT_ZVALUE);
                continue;
            }

            // Verify the node is still present in our map (it might have been removed)
            if (!nodeItems.contains(node->nodeFrontId)) {
                item->setZValue(NetworkNode::DEFAULT_ZVALUE);
                continue;
            }

            item->setZValue(NetworkNode::DEFAULT_ZVALUE);

            const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(node->nodeFrontId);
            for (const EdgeInfo& e : incidentEdges) {
                NetworkEdge* edge = edgeItems.value(qMakePair(node->nodeFrontId, e.destination));
                if (!edge && !directedEdges) {
                    edge = edgeItems.value(qMakePair(e.destination, node->nodeFrontId));
                }
                if (edge) edge->setZValue(NetworkEdge::DEFAULT_ZVALUE);
            }
        }
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

            if (node->nodeFrontId < 0) continue;
            if (!nodeItems.contains(node->nodeFrontId)) continue;

            const QVector<EdgeInfo> incidentEdges = dataHandler->getEdgesOf(node->nodeFrontId);

            // for all its incident edges, set the z value of the visual edge to selected
            for (const EdgeInfo& e : incidentEdges) {
                NetworkEdge* edge = edgeItems.value(qMakePair(node->nodeFrontId, e.destination));
                if (!edge && !directedEdges) {
                    edge = edgeItems.value(qMakePair(e.destination, node->nodeFrontId));
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
void NetSim::onLoadGraph() {
    QSettings settings;

    // get last used directory (fallback to home if none saved)
    QString lastDir = settings.value("lastGraphDir", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load Graph",
        lastDir,
        "Edge List Files (*.txt *.csv *.edges *.el *.tsv);;All Files (*)"
    );

    if (fileName.isEmpty()) return;

    // save the directory for next time
    settings.setValue("lastGraphDir", QFileInfo(fileName).absolutePath());

    if (fileName.isEmpty()) return;

    // start timer
    QElapsedTimer timer;
    timer.start();

    // open the file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Load Graph", "Could not open file:\n" + fileName);
        return;
    }

    // clear current graph
    clearGraph();

    bool contraction = (m_defaultLayoutAlgo == "compContract" || m_defaultLayoutAlgo == "ContractHighDegrees");

    // temporarily block signals to avoid scene updates mid-load
    scene->blockSignals(true);

    QTextStream in(&file);
    struct EdgeEntry { QString src, dst, label; };
    QList<EdgeEntry> edgeEntries;
    QSet<QString> uniqueNodeLabels;

    int lineNum = 0;
    int skipCount = 0;
    QMap<QString, int> nodeDegrees;

    // parse the file
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNum++;

        // skip comments and empty lines
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('%') || line.startsWith("//"))
            continue;

        QStringList parts = line.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            skipCount++;
            continue;
        }

        QString src = parts[0];
        QString dst = parts[1];
        QString weight = (parts.size() >= 3) ? parts[2] : "";

        edgeEntries.append({src, dst, weight});
        uniqueNodeLabels.insert(src);
        uniqueNodeLabels.insert(dst);

        // count node degrees
        nodeDegrees[src]++;
        if (!directedEdges) nodeDegrees[dst]++;
    }
    file.close();

    // add nodes to backend
    QMap<QString, int> labelToId;   
    for (const QString& label : uniqueNodeLabels) {
        int capacity = qMax(nodeDegrees.value(label, 1), 1);
        int nodeId = dataHandler->addNode(label, capacity);
        labelToId[label] = nodeId;
    }

    // add edges to backend
    for (const EdgeEntry& e : edgeEntries) {
        int srcId = labelToId.value(e.src, -1);
        int dstId = labelToId.value(e.dst, -1);
        if (srcId == -1 || dstId == -1) continue;

        // no multi
        if (!multiEdges) {
            if (dataHandler->edgeExists(srcId, dstId) ||
                (!directedEdges && dataHandler->edgeExists(dstId, srcId)))
                continue;
        }

        dataHandler->addEdge(srcId, dstId, e.label);
        if (!directedEdges)
            dataHandler->addEdge(dstId, srcId, e.label);
    }

    // do not add visual item if contracting
    if (!contraction) {
        for (auto it = labelToId.begin(); it != labelToId.end(); ++it) {
            const QString& label = it.key();
            int nodeId = it.value();

            // Add visual node at origin 
            NetworkNode* node = AddNodeAt(QPointF(0, 0), label, nodeDegrees.value(label, 1), nodeId);
        }

        // add visual edges
        for (const EdgeEntry& e : edgeEntries) {
            int srcId = labelToId.value(e.src, -1);
            int dstId = labelToId.value(e.dst, -1);
            if (srcId == -1 || dstId == -1) continue;

            AddVisualEdge(srcId, dstId, e.label, directedEdges);
        }
    }

    // stop timer after load
    qint64 elapsedUs = timer.nsecsElapsed() / 1000;
    QString msg = QString("Loaded %1 nodes, %2 edges from \"%3\"")
                      .arg(nodeItems.size())
                      .arg(edgeItems.size())
                      .arg(QFileInfo(fileName).fileName());
    if (skipCount > 0)
        msg += QString("  (%1 malformed line(s) skipped)").arg(skipCount);

    // format timer
    QString timeStr = elapsedUs < 1000
        ? QString("%1 µs").arg(elapsedUs)
        : elapsedUs < 1000000
            ? QString("%1 ms").arg(elapsedUs / 1000.0, 0, 'f', 2)
            : QString("%1 s").arg(elapsedUs / 1000000.0, 0, 'f', 3);

    msg += QString(" time to load: %1").arg(timeStr);

    ui->statusbar->showMessage(msg);

    // unblock and update
    scene->blockSignals(false);

    // default layout 
    if (m_defaultLayoutAlgo == "circular")
        algorithmPanel->runCircularLayout(false);
    else if (m_defaultLayoutAlgo == "spiral")
        algorithmPanel->runSpiralLayout(false);
    else if (m_defaultLayoutAlgo == "sfdp")
        algorithmPanel->runSFDPAlgo(false);
    else if (m_defaultLayoutAlgo == "compContract")
        algorithmPanel->runCompContract();
    else if (m_defaultLayoutAlgo == "ContractHighDegrees")
        algorithmPanel->runHighDegreeContract(false);

    
    if(!contraction) {
        updateEdges();
        updateSceneRect();
    }

    onResetView();
}


// ----------------------------------
// other netsim functions
// ----------------------------------

// Update all edges when nodes move
void NetSim::updateEdges() {
    if (updatingEdges) return;
    updatingEdges = true;

    QSet<NetworkEdge*> seen;

    // update all edges and mark them as seen
    for (NetworkEdge* edge : edgeItems) {
        if (edge && !seen.contains(edge)) {
            seen.insert(edge);
            edge->updatePosition();
        }
    }
    updateSceneRect();

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