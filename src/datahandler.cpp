#include "datahandler.h"
#include <algorithm>
#include <QDebug>

DataHandler::DataHandler() {}
DataHandler::~DataHandler() {}

// add a node where the id is the nodes index in the node array
int DataHandler::addNode(const QString& label) {
    int id;
    NodeInfo info;

    // starts with small capacity and 0 edges
    info.capacity = 4;
    info.degree = 0;

    // start of this nodes edges is end of edge array
    info.edge_index = edges.size();

    // if there are no empty node ids
    if(!emptyNodeIds.isEmpty()) {
        id = emptyNodeIds.pop();
        info = nodes[id];

        // add its info and label to their lists
        nodes[id] = info;
        nodeLabels[id] = label;
    } 
    // otherwise add a new node at the end of the list
    else {
        id = nodes.size();

        // add its info and label to their lists
        nodes.append(info);
        nodeLabels.append(label);
    }

    // Extend edges vector to hold the capacity
    ensureCapacity(info.edge_index + info.capacity);
    return id;
}

// remove a node and all of its edges leaving its space empty for another node
void DataHandler::removeNode(int nodeId) {
    if (nodeId < 0 || nodeId >= nodes.size() || nodes[nodeId].degree <= 0) return;

    // remove edges where the dest is this node, has to loop through all nodes * their degrees. :(
    QVector<QPair<int,int>> incoming;
    for (int src = 0; src < nodes.size(); ++src) {
        if (nodes[nodeId].degree <= 0|| src == nodeId) continue;
        const NodeInfo& info = nodes[src];
        for (int i = info.edge_index; i < info.edge_index + info.degree; ++i) {
            if (edges[i].destination == nodeId) {
                incoming.append(qMakePair(src, nodeId));
            }
        }
    }

    // remove the incoming edges
    for (const auto& p : incoming) {
        removeEdge(p.first, p.second);
    }

    // remove outgoing edges
    const QVector<EdgeInfo> outgoing = getEdgesOf(nodeId);
    for (const EdgeInfo& e : outgoing) {
        removeEdge(nodeId, e.destination);
    }

    // Mark node as inactive and recycle its ID
    nodes[nodeId].degree = -1;
    nodes[nodeId].edge_index = 0;
    nodes[nodeId].capacity = 0;
    nodeLabels[nodeId].clear();
    emptyNodeIds.push(nodeId);
}

// removes a node without looping through to delete all of its edges
void DataHandler::removeNodeNoEdges(int nodeId) {
    if (nodeId < 0 || nodeId >= nodes.size() || nodes[nodeId].degree <= 0) return;

    // Mark node as inactive and recycle its ID
    nodes[nodeId].degree = -1;
    nodes[nodeId].edge_index = 0;
    nodes[nodeId].capacity = 0;
    nodeLabels[nodeId].clear();
    emptyNodeIds.push(nodeId);
}

// set the label of a node using it id/index
void DataHandler::setNodeLabel(int nodeId, const QString& label) {
    if (nodeId >= 0 && nodeId < nodeLabels.size())
        nodeLabels[nodeId] = label;
}

// add an edge to the data with its label
void DataHandler::addEdge(int src, int dst, const QString& label) {
    if (src < 0 || src >= nodes.size() || dst < 0 || dst >= nodes.size()) return;

    // get the info and the position to insert the edge
    NodeInfo& info = nodes[src];
    int edge_position = findInsertPosition(src, dst);

    // Check if edge already exists, no multi-edges for now
    if (edge_position < info.edge_index + info.degree && edges[edge_position].destination == dst) {
        return;
    }

    // insert at pos, shifting elements to the right, rebalance if there is no more capacity
    if (info.degree >= info.capacity) {
        // rebalance to a new block with double capacity
        rebalanceNode(src, info.capacity * 2);

        // Recalculate pos after rebalance
        edge_position = findInsertPosition(src, dst);
    }
    // Shift elements right from pos to end of this node's segment
    for (int i = info.edge_index + info.degree; i > edge_position; --i) {
        edges[i] = edges[i-1];
    }
    edges[edge_position] = {dst, label};
    ++info.degree;
    ++totalEdges;
}

// remove an edge from the data and subtract it from the node
void DataHandler::removeEdge(int src, int dst) {
    if (src < 0 || src >= nodes.size()) return;
    NodeInfo& info = nodes[src];

    // get the edge position
    int edge_position = findInsertPosition(src, dst);

    // if it exists
    if (edge_position < info.edge_index + info.degree && edges[edge_position].destination == dst) {
        // Shift left all the edges after this one to fill the gap
        for (int i = edge_position; i < info.edge_index + info.degree - 1; ++i) {
            edges[i] = edges[i+1];
        }
        --info.degree;
        --totalEdges;

        // shrink capacity if density is too low
        if (info.capacity > 4 && info.degree <= info.capacity / 4) {
            rebalanceNode(src, info.capacity / 2);
        }
    }
}

// get all of the edges of a specific node
QVector<EdgeInfo> DataHandler::getEdgesOf(int nodeId) const {
    QVector<EdgeInfo> result;
    if (nodeId < 0 || nodeId >= nodes.size()) return result;

    const NodeInfo& info = nodes[nodeId];
    
    // node has no edges
    if (info.degree <= 0) return result;

    result.reserve(info.degree);

    // add each edge from the dge list of this node to the result
    for (int i = info.edge_index; i < info.edge_index + info.degree; ++i) {
        result.append(edges[i]);
    }
    return result;
}

// check if an edge exists
bool DataHandler::edgeExists(int src, int dst) const {
    if (src < 0 || src >= nodes.size()) return false;
    const NodeInfo& info = nodes[src];

    // find the position and check if the edge exists at that position
    int edge_position = findInsertPosition(src, dst);
    return (edge_position < info.edge_index + info.degree && edges[edge_position].destination == dst);
}

// set/change the label of an edge
void DataHandler::setEdgeLabel(int srcId, int dstId, const QString& label) {
    if (!nodeExists(srcId)) return;

    const NodeInfo& node = nodes[srcId];
    // loop throuhg the source nodes edges until we find the edge going to dest
    for (int i = node.edge_index; i < node.edge_index + node.degree; ++i) {
        if (edges[i].destination == dstId) {
            edges[i].label = label;
            return;
        }
    }
}

// find where to insert the edge into the edge array
int DataHandler::findInsertPosition(int nodeId, int dest) const {
    const NodeInfo& info = nodes[nodeId];

    // lower and upper bounds for hte binary search
    int lower_bound = info.edge_index;
    int upper_bound = info.edge_index + info.degree; 

    // binary search for the correct position to insert dest, keep edges sorted by destination
    while (lower_bound < upper_bound) {
        int mid = lower_bound + (upper_bound - lower_bound) / 2;
        if (edges[mid].destination < dest)
            lower_bound = mid + 1;
        else
            upper_bound = mid;
    }
    return lower_bound;
}

// change the capacity/amount of edges a node can hold
void DataHandler::rebalanceNode(int nodeId, int newCapacity) {
    NodeInfo& info = nodes[nodeId];
    if (newCapacity < info.degree) newCapacity = info.degree;

    // Create new block for this node's edges
    QVector<EdgeInfo> newEdges;
    newEdges.resize(newCapacity);

    // Copy existing edges
    for (int i = 0; i < info.degree; ++i) {
        newEdges[i] = edges[info.edge_index + i];
    }

    // rmove gaps from the edge list
    compact();

    // get the differencr, wether to expand or shrink
    int delta = newCapacity - info.capacity;
    if (delta > 0) {
        // ensure the edge list is big engough
        ensureCapacity(edges.size() + delta);

        // shift elements after this nodes block
        for (int i = nodes.size() - 1; i > nodeId; --i) {
            NodeInfo& next = nodes[i];
            if (next.edge_index > info.edge_index) {
                // Shift entire block of next node by delta
                for (int j = next.edge_index + next.degree - 1; j >= next.edge_index; --j) {
                    edges[j + delta] = edges[j];
                }
                next.edge_index += delta;
            }
        }

        // Fill the new gap with empty slots
        for (int i = info.edge_index + info.degree; i < info.edge_index + newCapacity; ++i) {
            edges[i] = { -1, QString() }; 
        }
        info.capacity = newCapacity;
    } else if (delta < 0) {
        int shift = -delta;

        // Move edges of later nodes left
        for (int i = nodeId + 1; i < nodes.size(); ++i) {
            NodeInfo& next = nodes[i];
            if (next.edge_index > info.edge_index) {
                for (int j = next.edge_index; j < next.edge_index + next.degree; ++j) {
                    edges[j - shift] = edges[j];
                }
                next.edge_index -= shift;
            }
        }
        info.capacity = newCapacity;
    }
}

// Rebuild the edge array without gaps
void DataHandler::compact() {
    QVector<EdgeInfo> newEdges;
    int newPos = 0;

    // loop through every node removing gaps
    for (int i = 0; i < nodes.size(); ++i) {
        NodeInfo& info = nodes[i];

        // covers nodes with no edges
        if (info.degree <= 0) {
            info.edge_index = newPos;
            info.capacity = info.degree; 
            continue;
        }

        // save the old index before overwriting it
        int oldIndex = info.edge_index;
        info.edge_index = newPos;

        // Copy this node's edges
        for (int j = 0; j < info.degree; ++j) {
            newEdges.append(edges[info.edge_index + j]);
        }
        newPos += info.degree;

        // after the compact, the capacity is the same as the degree
        info.capacity = info.degree; 
    }
    edges = newEdges;
    totalEdges = newEdges.size();
}

// clear all data
void DataHandler::clear() {
    nodes.clear();
    edges.clear();
    nodeLabels.clear();
    totalEdges = 0;
    emptyNodeIds.clear();
}

// resize the edge array if we need more space
void DataHandler::ensureCapacity(int newSize) {
    if (edges.size() < newSize) {
        edges.resize(newSize);
    }
}