#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QWheelEvent>

// Forward declarations to avoid circular dependencies
class Graph;
class Node;

/**
 * @class GraphWidget
 * @brief Custom QGraphicsView widget for network visualization
 * 
 * Extends QGraphicsView to provide custom mouse interactions
 * for the network simulator.
 */
class GraphWidget : public QGraphicsView {
    Q_OBJECT  // Qt macro required for signals/slots
    
public:
    /**
     * @brief Constructor for GraphWidget
     * @param parent Parent widget (optional)
     */
    explicit GraphWidget(QWidget* parent = nullptr);
    
protected:
    /**
     * @brief Handles mouse press events for node selection
     * @param event Mouse event details
     */
    void mousePressEvent(QMouseEvent* event) override;
    
    /**
     * @brief Handles mouse move events for dragging
     * @param event Mouse event details
     */
    void mouseMoveEvent(QMouseEvent* event) override;
    
    /**
     * @brief Handles mouse wheel events for zooming
     * @param event Wheel event details
     */
    void wheelEvent(QWheelEvent* event) override;
    
private:
    QGraphicsScene* scene;  ///< Graphics scene containing all items
    Graph* graph;           ///< Network graph data structure
    Node* selectedNode;     ///< Currently selected node for interaction
};

#endif // GRAPHWIDGET_H