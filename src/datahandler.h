#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <QVector>
#include <QString>
#include <QPair>
#include <QStack>

// structure of the edge has is destination node and label
struct EdgeInfo {
    int destination;
    QString label;
};

// node structure has the index of its first edge, the capacity of its edge list, and its degree
struct NodeInfo {
        int edge_index;
        int capacity;   
        int degree;
        int x_pos;
        int y_pos;
    };

class DataHandler {
public:
    explicit DataHandler();
    ~DataHandler();

    // Node operations
    int addNode(const QString& label, int x_pos=0, int y_pos=0);
    void removeNode(int nodeId);
    void removeNodeNoEdges(int nodeId);
    int nodeCount() const { return nodes.size() - emptyNodeIds.size(); }
    QString nodeLabel(int nodeId) const { return nodeLabels.value(nodeId); }
    void setNodeLabel(int nodeId, const QString& label);
    bool nodeExists(int nodeId) const { return nodeId >= 0 && nodeId < nodes.size() && nodes[nodeId].degree != -1; }
    const QVector<NodeInfo>* getAllNodes() const { return &nodes; }
    const NodeInfo* getNode(int nodeId) const { return (nodeId >= 0 && nodeId < nodes.size()) ? &nodes[nodeId] : nullptr; }

    // Edge operations
    void addEdge(int src, int dst, const QString& label);
    void removeEdge(int src, int dst);
    QVector<EdgeInfo> getEdgesOf(int nodeId) const;
    const QVector<EdgeInfo>* getAllEdges() const { return &edges; }

    bool edgeExists(int src, int dst) const;
    int edgeCount() const { return totalEdges; }

    void compact();

private:
    QVector<NodeInfo> nodes;
    QVector<EdgeInfo> edges;
    QVector<QString> nodeLabels;
    int totalEdges = 0;
    QStack<int> emptyNodeIds;

    // some helpers
    int findInsertPosition(int nodeId, int dst) const;
    void rebalanceNode(int nodeId, int newCapacity);
    void ensureCapacity(int newSize);
};

#endif // DATAHANDLER_H