// graphwidget.h
class GraphWidget : public QGraphicsView {
    Q_OBJECT
    
public:
    GraphWidget(QWidget* parent = nullptr);
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    QGraphicsScene* scene;
    Graph* graph;
    Node* selectedNode;
    
};