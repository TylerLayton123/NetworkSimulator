#ifndef ALGORITHMPANEL_H
#define ALGORITHMPANEL_H

#include <QWidget>
#include <QList>
#include <QVector>
#include <QPointF>
#include <limits>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
#include <QStackedWidget>
#include <QFont>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QQueue>
#include <QStack>
#include <QMap>
#include <QSet>
#include <functional>
#include <limits>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <QSettings>

class NetworkNode;
class NetworkEdge;
class QTextEdit;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;

// Parameters resolved from the dialog before running a traversal algorithm
struct AlgoParams {
    NetworkNode* source = nullptr;
    NetworkNode* target = nullptr;
};

// Parameters for the SFDP layout dialog
struct SFDPParams {
    int iterations = 100;
    double K = 150.0;  // ideal edge length in scene pixels
    double C = 0.2;    // repulsion constant
    double tol = 0.01;    // convergence tolerance
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

private slots:
    void sfdpStep();

private:
    // ── Graph data ─────────────────────────────────────────────
    QList<NetworkNode*> m_nodes;
    QList<NetworkEdge*> m_edges;
    NetworkNode*        m_sourceNode = nullptr;

    // ── Widgets ────────────────────────────────────────────────
    QTextEdit*      m_output      = nullptr;
    QLabel*         m_sourceInfo  = nullptr;
    QStackedWidget* m_stack       = nullptr;
    QPushButton*    m_searchBtn   = nullptr;
    QPushButton*    m_visualsBtn  = nullptr;
    QPushButton*    m_sfdpStopBtn = nullptr;

    // ── SFDP animation state ───────────────────────────────────
    QTimer*          m_sfdpTimer    = nullptr;
    int              m_sfdpIter     = 0;
    int              m_sfdpMaxIter  = 100;
    double           m_sfdpStep     = 1.0;
    double           m_sfdpEnergy   = std::numeric_limits<double>::max();
    int              m_sfdpProgress = 0;
    double           m_sfdpK        = 150.0;
    double           m_sfdpC        = 0.2;
    double           m_sfdpTol      = 1.0;
    bool             m_sfdpStopFlag = false;
    int              m_sfdpN        = 0;
    QVector<QPointF> m_sfdpPos;       // current positions in scene coords
    QVector<bool>    m_sfdpAdj;       // flat N×N adjacency matrix

    // ── UI build ───────────────────────────────────────────────
    void     buildUI();
    void     showSearchPage();
    void     showVisualPage();
    QWidget* buildAlgoPage(const QList<QPair<QString,QString>>& algos);

    // ── Dispatch and dialogs ───────────────────────────────────
    void runAlgorithm(const QString& id);
    void printResult(const QString& title, const QString& body);

    bool askParams(const QString& algoName, bool needsSource,
                   bool needsTarget, AlgoParams& out);
    bool askSFDPParams(SFDPParams& out);

    void runSFDP(const SFDPParams& p);
    void stopSFDP();

    // ── Search / Analysis ──────────────────────────────────────
    QString algoBFS(NetworkNode* source, NetworkNode* target);
    QString algoDFS(NetworkNode* source, NetworkNode* target);
    QString algoDijkstra(NetworkNode* source, NetworkNode* target);
    QString algoCycleDetection();
    QString algoConnectedComponents();
    QString algoTopoSort();

    // ── Metrics / Structural ───────────────────────────────────
    QString algoMST();
    QString algoDegreeStats();
    QString algoBipartite();
    QString algoGraphDensity();

    // ── Helpers ────────────────────────────────────────────────
    NetworkNode* sourceOrFirst() const;
    double       edgeWeight(NetworkEdge* e) const;
    NetworkNode* neighbour(NetworkEdge* edge, NetworkNode* from) const;
};

#endif // ALGORITHMPANEL_H