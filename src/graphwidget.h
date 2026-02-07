#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QWheelEvent>

// Forward declarations 
class Graph;
class Node;

// Custom QGraphicsView widget for network visualization
// Extends QGraphicsView to provide custom mouse interactions
class GraphWidget : public QGraphicsView {
    Q_OBJECT  
    
public:
    // Constructor for GraphWidget
    explicit GraphWidget(QWidget* parent = nullptr);
    
protected:
    // Handles mouse press events for node selection
    void mousePressEvent(QMouseEvent* event) override;
    
    // Handles mouse move events for dragging
    void mouseMoveEvent(QMouseEvent* event) override;
    
    // Handles mouse wheel events for zooming
    void wheelEvent(QWheelEvent* event) override;
    
private:
    QGraphicsScene* scene;  
    Graph* graph;           
    Node* selectedNode;     
};

#endif 