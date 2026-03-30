#ifndef BACKEND_H
#define BACKEND_H

#include <QVector>
#include <QString>
#include <QPair>

// structure of the edge has is destination node and label
struct Edge {
    int destination;
    QString label;
};

class PCSRGraph {
public:
    explicit PCSRGraph();
    ~PCSRGraph();

    // Node operations
    int addNode(const QString& label);
    void removeNode(int nodeId);
    int nodeCount() const { return nodes.size(); }
    QString nodeLabel(int nodeId) const { return nodeLabels.value(nodeId); }
    void setNodeLabel(int nodeId, const QString& label);

    // Edge operations
    void addEdge(int src, int dst, const QString& label);
    void removeEdge(int src, int dst);
    QVector<Edge> getEdgesOf(int nodeId) const;

    bool edgeExists(int src, int dst) const;
    int edgeCount() const { return totalEdges; }

    void compact();

private:
    // node structure has the index of its first edge, the capacity of its edge list, and its degree
    struct NodeInfo {
        int edge_index;
        int capacity;   
        int degree;
    };

    QVector<NodeInfo> nodes;
    QVector<Edge> edges;
    QVector<QString> nodeLabels;
    int totalEdges = 0;

    // some helpers
    int findInsertPosition(int nodeId, int dst) const;
    void rebalanceNode(int nodeId, int newCapacity);
    void ensureCapacity(int newSize);
};

#endif // BACKEND_H