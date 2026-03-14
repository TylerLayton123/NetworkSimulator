#ifndef ALGORITHMPANEL_H
#define ALGORITHMPANEL_H

#include <QWidget>
#include <QList>

class NetworkNode;
class NetworkEdge;
class QTextEdit;
class QLabel;
class QPushButton;
class QStackedWidget;

class AlgorithmPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AlgorithmPanel(QWidget* parent = nullptr);

    void setData(const QList<NetworkNode*>& nodes, const QList<NetworkEdge*>& edges);
    void setSourceNode(NetworkNode* node);

signals:
    void requestHighlightNodes(const QList<NetworkNode*>& nodes);
    void requestHighlightEdges(const QList<NetworkEdge*>& edges);

private:
    QList<NetworkNode*> m_nodes;
    QList<NetworkEdge*> m_edges;
    NetworkNode*        m_sourceNode = nullptr;

    QTextEdit*      m_output     = nullptr;
    QLabel*         m_sourceInfo = nullptr;
    QStackedWidget* m_stack      = nullptr;
    QPushButton*    m_searchBtn  = nullptr;
    QPushButton*    m_visualizationBtn = nullptr;

    void     buildUI();
    void     showSearchPage();
    void     showVisualizationPage();
    QWidget* buildAlgoPage(const QList<QPair<QString,QString>>& algos);
    void     runAlgorithm(const QString& id);
    void     printResult(const QString& title, const QString& body);

    QString algoBFS();
    QString algoDFS();
    QString algoDijkstra();
    QString algoCycleDetection();
    QString algoConnectedComponents();
    QString algoTopoSort();
    QString algoMST();
    QString algoDegreeStats();
    QString algoBipartite();
    QString algoGraphDensity();

    NetworkNode* sourceOrFirst() const;
    double       edgeWeight(NetworkEdge* e) const;
    NetworkNode* neighbour(NetworkEdge* edge, NetworkNode* from) const;
};

#endif // ALGORITHMPANEL_H