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
#include <QElapsedTimer>
#include "datahandler.h"

class NetworkNode;
class NetworkEdge;
class DataHandler;
class QTextEdit;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;

// Parameters resolved from the dialog before running a traversal algorithm
struct AlgoParams {
    int sourceId;
    int targetId;
};

// Parameters for the SFDP layout dialog
struct SFDPParams {
    int iterations = 100;
    double K = 150.0;  // ideal edge length in scene pixels
    double C = 0.2;    // repulsion constant
    double tol = 0.01;    // convergence tolerance
};

// parameters for the circular layout dialog, spacing between nodes
struct CircularParams {
    double spacing = 0.0;   
};

// spiral parameters
struct SpiralParams {
    qreal spacing = 100.0;
    qreal radiusGrowth = 50.0;
};

class AlgorithmPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AlgorithmPanel(QWidget* parent = nullptr);

    void setData(QHash<int, NetworkNode*>* nodes, QHash<QPair<int,int>, NetworkEdge*>* edges, DataHandler* dataHandler);
    void setSourceNode(int nodeId);
    void runCircularLayout(bool askUser);
    void runSpiralLayout(bool askUser);

    // default params
    SFDPParams m_sfdpParams;
    CircularParams m_circularParams;
    SpiralParams m_spiralParams;

signals:
    void requestHighlightNodes(QHash<int, NetworkNode*>& nodes);
    void requestHighlightEdges(QHash<QPair<int,int>, NetworkEdge*>& edges);
    void newSceneSize(int newSceneSize);

private slots:
    void sfdpStep();
    

private:
    // ── Graph data ─────────────────────────────────────────────
    QHash<int, NetworkNode*>* m_nodeItems;
    QHash<QPair<int,int>, NetworkEdge*>* m_edgeItems;
    int m_sourceId = -1;
    const QVector<NodeInfo>* nodeData = nullptr;
    const QVector<EdgeInfo>* edgeData = nullptr;
    DataHandler* m_dataHandler = nullptr;

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
    void buildUI();
    void showSearchPage();
    void showVisualPage();
    QWidget* buildAlgoPage(const QList<QPair<QString,QString>>& algos);

    // ── Dispatch and dialogs ───────────────────────────────────
    void printResult(const QString& title, const QString& body);
    void runAlgorithm(const QString& id);

    bool askParams(const QString& algoName, bool needsSource, bool needsTarget, AlgoParams& out);
    bool askSFDPParams(SFDPParams& out);

    // ── Visualization algorithms ───────────────────────────────
    void runSFDP(const SFDPParams& p);
    void stopSFDP();
    QString algoCircularLayout(bool askUser = true);
    bool askCircularParams(CircularParams& out);
    QString algoSpiralLayout(bool askUser = true);
    bool askSpiralParams(SpiralParams& out);

    // ── Search / Analysis ──────────────────────────────────────
    QString algoBFS(int sourceId, int targetId);
    QString algoDFS(int sourceId, int targetId);
    QString algoDijkstra(int sourceId, int targetId);
    QString algoConnectedComponents();


    // ── Helpers ────────────────────────────────────────────────
    int sourceOrFirst() const;
    // double edgeWeight(NetworkEdge* e) const;
    // NetworkNode* neighbour(NetworkEdge* edge, NetworkNode* from) const;
};

#endif // ALGORITHMPANEL_H

