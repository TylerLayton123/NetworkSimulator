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

struct AlgoParams {
    NetworkNode* source = nullptr; 
    NetworkNode* target = nullptr; 
};

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
    QPushButton*    m_metricsBtn = nullptr;

    void     buildUI();
    void     showSearchPage();
    void     showMetricsPage();
    QWidget* buildAlgoPage(const QList<QPair<QString,QString>>& algos);
    void     runAlgorithm(const QString& id);
    void     printResult(const QString& title, const QString& body);


    bool askParams(const QString& algoName, bool needsSource, bool needsTarget, AlgoParams& out);

    // ── Search / Analysis ──────────────────────────────────────
    QString algoBFS(NetworkNode* source, NetworkNode* target);
    QString algoDFS(NetworkNode* source, NetworkNode* target);
    QString algoDijkstra(NetworkNode* source, NetworkNode* target);
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