#include "mutablegraph.h"
#include "componentmanager.h"

#include "shared/utils/utils.h"

MutableGraph::~MutableGraph()
{
    // Ensure no transactions are in progress
    std::unique_lock<std::mutex> lock(_mutex);
}

void MutableGraph::clear()
{
    beginTransaction();

    for(auto nodeId : nodeIds())
        removeNode(nodeId);

    _updateRequired = true;
    endTransaction();

    // Removing all the nodes should remove all the edges
    Q_ASSERT(numEdges() == 0);

    _n.clear();
    _n.resize(0);
    _e.clear();
    _e.resize(0);

    Graph::clear();
}

const std::vector<NodeId>& MutableGraph::nodeIds() const
{
    return _nodeIds;
}

int MutableGraph::numNodes() const
{
    return static_cast<int>(_nodeIds.size());
}

const INode& MutableGraph::nodeById(NodeId nodeId) const
{
    Q_ASSERT(_n._nodeIdsInUse[nodeId]);
    return _n._nodes[nodeId];
}

bool MutableGraph::containsNodeId(NodeId nodeId) const
{
    return _n._nodeIdsInUse[nodeId];
}

NodeIdDistinctSetCollection::Type MutableGraph::typeOf(NodeId nodeId) const
{
    return _n._mergedNodeIds.typeOf(nodeId);
}

ConstNodeIdDistinctSet MutableGraph::mergedNodeIdsForNodeId(NodeId nodeId) const
{
    return ConstNodeIdDistinctSet(nodeId, &_n._mergedNodeIds);
}

NodeId MutableGraph::addNode()
{
    if(!_unusedNodeIds.empty())
    {
        auto unusedNodeId = _unusedNodeIds.front();
        _unusedNodeIds.pop_front();

        return addNode(unusedNodeId);
    }

    return addNode(nextNodeId());
}

void MutableGraph::reserveNodeId(NodeId nodeId)
{
    if(nodeId < nextNodeId())
        return;

    Graph::reserveNodeId(nodeId);
    _n.resize(nextNodeId());
}

NodeId MutableGraph::addNode(NodeId nodeId)
{
    Q_ASSERT(!nodeId.isNull());

    beginTransaction();

    // The requested ID is not available or is out of range, so resize and append
    if(nodeId >= nextNodeId() || _n._nodeIdsInUse[nodeId])
    {
        nodeId = nextNodeId();
        reserveNodeId(nodeId);
    }

    _n._nodeIdsInUse[nodeId] = true;
    auto& node = _n._nodes[nodeId];
    node._id = nodeId;
    node._inEdgeIds.setCollection(&_e._inEdgeIdsCollection);
    node._outEdgeIds.setCollection(&_e._outEdgeIdsCollection);

    emit nodeAdded(this, nodeId);
    _updateRequired = true;
    endTransaction();

    return nodeId;
}

NodeId MutableGraph::addNode(const INode& node)
{
    return addNode(node.id());
}

void MutableGraph::removeNode(NodeId nodeId)
{
    Q_ASSERT(containsNodeId(nodeId));

    beginTransaction();

    // Remove all edges that touch this node
    for(auto edgeId : inEdgeIdsForNodeId(nodeId).copy())
        removeEdge(edgeId);

    // ...do in and out separately in case an edge is in both
    for(auto edgeId : outEdgeIdsForNodeId(nodeId).copy())
        removeEdge(edgeId);

    _n._mergedNodeIds.remove(NodeIdDistinctSetCollection::SetId(), nodeId);
    _n._nodeIdsInUse[nodeId] = false;
    _unusedNodeIds.push_back(nodeId);

    emit nodeRemoved(this, nodeId);
    _updateRequired = true;
    endTransaction();
}

const std::vector<EdgeId>& MutableGraph::edgeIds() const
{
    return _edgeIds;
}

int MutableGraph::numEdges() const
{
    return static_cast<int>(_edgeIds.size());
}

const IEdge& MutableGraph::edgeById(EdgeId edgeId) const
{
    Q_ASSERT(_e._edgeIdsInUse[edgeId]);
    return _e._edges[edgeId];
}

bool MutableGraph::containsEdgeId(EdgeId edgeId) const
{
    return _e._edgeIdsInUse[edgeId];
}

EdgeIdDistinctSetCollection::Type MutableGraph::typeOf(EdgeId edgeId) const
{
    return _e._mergedEdgeIds.typeOf(edgeId);
}

ConstEdgeIdDistinctSet MutableGraph::mergedEdgeIdsForEdgeId(EdgeId edgeId) const
{
    return ConstEdgeIdDistinctSet(edgeId, &_e._mergedEdgeIds);
}

EdgeIdDistinctSets MutableGraph::edgeIdsForNodeId(NodeId nodeId) const
{
    EdgeIdDistinctSets set;
    auto& node = _n._nodes[nodeId];

    set.add(node._inEdgeIds);
    set.add(node._outEdgeIds);

    return set;
}

EdgeIdDistinctSet MutableGraph::inEdgeIdsForNodeId(NodeId nodeId) const
{
    return _n._nodes[nodeId]._inEdgeIds;
}

EdgeIdDistinctSet MutableGraph::outEdgeIdsForNodeId(NodeId nodeId) const
{
    return _n._nodes[nodeId]._outEdgeIds;
}

EdgeId MutableGraph::addEdge(NodeId sourceId, NodeId targetId)
{
    if(!_unusedEdgeIds.empty())
    {
        auto unusedEdgeId = _unusedEdgeIds.front();
        _unusedEdgeIds.pop_front();

        return addEdge(unusedEdgeId, sourceId, targetId);
    }

    return addEdge(nextEdgeId(), sourceId, targetId);
}

void MutableGraph::reserveEdgeId(EdgeId edgeId)
{
    if(edgeId < nextEdgeId())
        return;

    Graph::reserveEdgeId(edgeId);
    _e.resize(nextEdgeId());
}

NodeId MutableGraph::mergeNodes(NodeId nodeIdA, NodeId nodeIdB)
{
    return _n._mergedNodeIds.add(nodeIdA, nodeIdB);
}

EdgeId MutableGraph::mergeEdges(EdgeId edgeIdA, EdgeId edgeIdB)
{
    return _e._mergedEdgeIds.add(edgeIdA, edgeIdB);
}

EdgeId MutableGraph::addEdge(EdgeId edgeId, NodeId sourceId, NodeId targetId)
{
    Q_ASSERT(!edgeId.isNull());
    Q_ASSERT(_n._nodeIdsInUse[sourceId]);
    Q_ASSERT(_n._nodeIdsInUse[targetId]);

    beginTransaction();

    // The requested ID is not available or is out of range, so resize and append
    if(edgeId >= nextEdgeId() || _e._edgeIdsInUse[edgeId])
    {
        edgeId = nextEdgeId();
        reserveEdgeId(edgeId);
    }

    _e._edgeIdsInUse[edgeId] = true;
    auto& edge = _e._edges[edgeId];
    edge._id = edgeId;
    edge._sourceId = sourceId;
    edge._targetId = targetId;

    _n._nodes[sourceId]._outEdgeIds.add(edgeId);
    _n._nodes[targetId]._inEdgeIds.add(edgeId);

    auto undirectedEdge = UndirectedEdge(sourceId, targetId);
    if(!u::contains(_e._connections, undirectedEdge))
        _e._connections.emplace(undirectedEdge, EdgeIdDistinctSet(&_e._mergedEdgeIds));

    _e._connections[undirectedEdge].add(edgeId);

    emit edgeAdded(this, edgeId);
    _updateRequired = true;
    endTransaction();

    return edgeId;
}

EdgeId MutableGraph::addEdge(const IEdge& edge)
{
    return addEdge(edge.id(), edge.sourceId(), edge.targetId());
}

void MutableGraph::removeEdge(EdgeId edgeId)
{
    Q_ASSERT(containsEdgeId(edgeId));

    beginTransaction();

    // Remove all node references to this edge
    const auto& edge = _e._edges[edgeId];

    _n._nodes[edge.sourceId()]._outEdgeIds.remove(edgeId);
    _n._nodes[edge.targetId()]._inEdgeIds.remove(edgeId);

    auto undirectedEdge = UndirectedEdge(edge.sourceId(), edge.targetId());
    auto& connection = _e._connections[undirectedEdge];
    Q_ASSERT(connection.size() > 0);
    connection.remove(edgeId);

    if(connection.size() == 0)
        _e._connections.erase(undirectedEdge);

    _e._edgeIdsInUse[edgeId] = false;
    _unusedEdgeIds.push_back(edgeId);

    emit edgeRemoved(this, edgeId);
    _updateRequired = true;
    endTransaction();
}

// Move the edges to connect to nodeId
template<typename C> static void moveEdgesTo(MutableGraph& graph, NodeId nodeId,
                                             const C& inEdgeIds,
                                             const C& outEdgeIds)
{
    // Don't bother emitting signals for edges that are moving
    bool wasBlocked = graph.blockSignals(true);

    for(auto edgeIdToMove : inEdgeIds)
    {
        auto sourceId = graph.edgeById(edgeIdToMove).sourceId();
        graph.removeEdge(edgeIdToMove);
        graph.addEdge(edgeIdToMove, sourceId, nodeId);
    }

    for(auto edgeIdToMove : outEdgeIds)
    {
        auto targetId = graph.edgeById(edgeIdToMove).targetId();
        graph.removeEdge(edgeIdToMove);
        graph.addEdge(edgeIdToMove, nodeId, targetId);
    }

    graph.blockSignals(wasBlocked);
}

void MutableGraph::contractEdge(EdgeId edgeId)
{
    // Can't contract an edge that doesn't exist
    if(!containsEdgeId(edgeId))
        return;

    beginTransaction();

    auto& edge = edgeById(edgeId);
    NodeId nodeId, nodeIdToMerge;
    std::tie(nodeId, nodeIdToMerge) = std::minmax(edge.sourceId(), edge.targetId());

    removeEdge(edgeId);
    moveEdgesTo(*this, nodeId,
                inEdgeIdsForNodeId(nodeIdToMerge).copy(),
                outEdgeIdsForNodeId(nodeIdToMerge).copy());
    mergeNodes(nodeId, nodeIdToMerge);

    _updateRequired = true;
    endTransaction();
}

void MutableGraph::contractEdges(const EdgeIdSet& edgeIds)
{
    if(edgeIds.empty())
        return;

    beginTransaction();

    // Divide into components, but ignore any edges that aren't being contracted,
    // so that each component represents a set of nodes that will be merged
    ComponentManager componentManager(*this, nullptr,
    [&edgeIds](EdgeId edgeId)
    {
        return !u::contains(edgeIds, edgeId);
    });

    removeEdges(edgeIds);

    for(auto componentId : componentManager.componentIds())
    {
        auto component = componentManager.componentById(componentId);

        // Nothing to contract
        if(component->numEdges() == 0)
            continue;

        auto nodeId = *std::min_element(component->nodeIds().begin(),
                                        component->nodeIds().end());

        moveEdgesTo(*this, nodeId,
                    inEdgeIdsForNodeIds(component->nodeIds()).copy(),
                    outEdgeIdsForNodeIds(component->nodeIds()).copy());

        for(auto nodeIdToMerge : component->nodeIds())
            mergeNodes(nodeId, nodeIdToMerge);
    }

    _updateRequired = true;
    endTransaction();
}

void MutableGraph::cloneFrom(const Graph& other)
{
    beginTransaction();

    const auto* mutableOther = dynamic_cast<const MutableGraph*>(&other);
    Q_ASSERT(mutableOther != nullptr);

    // Store the differences between the graphs
    auto diff = diffTo(*mutableOther);

    _n             = mutableOther->_n;
    _nodeIds       = mutableOther->_nodeIds;
    _unusedNodeIds = mutableOther->_unusedNodeIds;
    reserveNodeId(mutableOther->largestNodeId());

    _e             = mutableOther->_e;
    _edgeIds       = mutableOther->_edgeIds;
    _unusedEdgeIds = mutableOther->_unusedEdgeIds;
    reserveEdgeId(mutableOther->largestEdgeId());

    // Reset collection pointers to collections in this
    for(auto& node : _n._nodes)
    {
        node._inEdgeIds.setCollection(&_e._inEdgeIdsCollection);
        node._outEdgeIds.setCollection(&_e._outEdgeIdsCollection);
    }

    for(auto& connection : _e._connections)
        connection.second.setCollection(&_e._mergedEdgeIds);

    // Signal all the changes based on the diff before we cloned
    for(NodeId nodeId : diff._nodesAdded)
        emit nodeAdded(this, nodeId);

    for(EdgeId edgeId : diff._edgesAdded)
        emit edgeAdded(this, edgeId);

    for(EdgeId edgeId : diff._edgesRemoved)
        emit edgeRemoved(this, edgeId);

    for(NodeId nodeId : diff._nodesRemoved)
        emit nodeRemoved(this, nodeId);

    _updateRequired = true;
    endTransaction();
}

MutableGraph::Diff MutableGraph::diffTo(const MutableGraph& other)
{
    MutableGraph::Diff diff;

    auto maxNodeId = std::max(nextNodeId(), other.nextNodeId());
    for(NodeId nodeId(0); nodeId < maxNodeId; ++nodeId)
    {
        if(nodeId < nextNodeId() && nodeId < other.nextNodeId())
        {
            if(_n._nodeIdsInUse[nodeId] && !other._n._nodeIdsInUse[nodeId])
                diff._nodesRemoved.push_back(nodeId);
            else if(!_n._nodeIdsInUse[nodeId] && other._n._nodeIdsInUse[nodeId])
                diff._nodesAdded.push_back(nodeId);
        }
        else if(nodeId < nextNodeId() && _n._nodeIdsInUse[nodeId])
            diff._nodesRemoved.push_back(nodeId);
        else if(nodeId < other.nextNodeId() && other._n._nodeIdsInUse[nodeId])
            diff._nodesAdded.push_back(nodeId);
    }

    auto maxEdgeId = std::max(nextEdgeId(), other.nextEdgeId());
    for(EdgeId edgeId(0); edgeId < maxEdgeId; ++edgeId)
    {
        if(edgeId < nextEdgeId() && edgeId < other.nextEdgeId())
        {
            if(_e._edgeIdsInUse[edgeId] && !other._e._edgeIdsInUse[edgeId])
                diff._edgesRemoved.push_back(edgeId);
            else if(!_e._edgeIdsInUse[edgeId] && other._e._edgeIdsInUse[edgeId])
                diff._edgesAdded.push_back(edgeId);
        }
        else if(edgeId < nextEdgeId() && _e._edgeIdsInUse[edgeId])
            diff._edgesRemoved.push_back(edgeId);
        else if(edgeId < other.nextEdgeId() && other._e._edgeIdsInUse[edgeId])
            diff._edgesAdded.push_back(edgeId);
    }

    return diff;
}

void MutableGraph::beginTransaction()
{
    if(_graphChangeDepth++ <= 0)
    {
        emit graphWillChange(this);
        _mutex.lock();
    }
}

void MutableGraph::endTransaction()
{
    Q_ASSERT(_graphChangeDepth > 0);
    if(--_graphChangeDepth <= 0)
    {
        update();
        _mutex.unlock();
        emit graphChanged(this);
        clearPhase();
    }
}

void MutableGraph::update()
{
    if(!_updateRequired)
        return;

    _updateRequired = false;

    _nodeIds.clear();
    _unusedNodeIds.clear();
    for(NodeId nodeId(0); nodeId < nextNodeId(); ++nodeId)
    {
        if(_n._nodeIdsInUse[nodeId])
            _nodeIds.emplace_back(nodeId);
        else
            _unusedNodeIds.emplace_back(nodeId);
    }

    _edgeIds.clear();
    _unusedEdgeIds.clear();
    for(EdgeId edgeId(0); edgeId < nextEdgeId(); ++edgeId)
    {
        if(_e._edgeIdsInUse[edgeId])
            _edgeIds.emplace_back(edgeId);
        else
            _unusedEdgeIds.emplace_back(edgeId);
    }
}
