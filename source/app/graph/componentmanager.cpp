#include "componentmanager.h"

#include "shared/utils/thread.h"
#include "shared/utils/container.h"
#include "shared/graph/elementid_debug.h"

#include <map>
#include <queue>

ComponentManager::ComponentManager(Graph& graph,
                                   const NodeConditionFn& nodeFilter,
                                   const EdgeConditionFn& edgeFilter) :
    _nextComponentId(0),
    _nodesComponentId(graph),
    _edgesComponentId(graph)
{
    // Ignore all multi-elements
    addNodeFilter([&graph](NodeId nodeId) { return graph.typeOf(nodeId) == MultiElementType::Tail; });
    addEdgeFilter([&graph](EdgeId edgeId) { return graph.typeOf(edgeId) == MultiElementType::Tail; });

    if(nodeFilter)
        addNodeFilter(nodeFilter);

    if(edgeFilter)
        addEdgeFilter(edgeFilter);

    connect(&graph, &Graph::graphChanged, this, &ComponentManager::onGraphChanged, Qt::DirectConnection);

    graph.update();
    update(&graph);
}

ComponentManager::~ComponentManager()
{
    // Let the ComponentArrays know that we're going away
    for(auto componentArray : _componentArrays)
        componentArray->invalidate();
}

ComponentIdSet ComponentManager::assignConnectedElementsComponentId(const Graph* graph,
        NodeId rootId, ComponentId componentId,
        NodeArray<ComponentId>& nodesComponentId,
        EdgeArray<ComponentId>& edgesComponentId)
{
    std::queue<NodeId> nodeIds;
    ComponentIdSet oldComponentIdsAffected;

    nodeIds.push(rootId);

    while(!nodeIds.empty())
    {
        auto nodeId = nodeIds.front();
        nodeIds.pop();
        oldComponentIdsAffected.insert(_nodesComponentId[nodeId]);
        for(auto mergedNodeId : graph->mergedNodeIdsForNodeId(nodeId))
            nodesComponentId[mergedNodeId] = componentId;

        for(auto edgeId : graph->edgeIdsForNodeId(nodeId))
        {
            if(edgeIdFiltered(edgeId))
                continue;

            for(auto mergedEdgeId : graph->mergedEdgeIdsForEdgeId(edgeId))
                edgesComponentId[mergedEdgeId] = componentId;

            auto oppositeNodeId = graph->edgeById(edgeId).oppositeId(nodeId);

            if(nodesComponentId[oppositeNodeId] != componentId)
            {
                nodeIds.push(oppositeNodeId);
                for(auto mergedNodeId : graph->mergedNodeIdsForNodeId(oppositeNodeId))
                    nodesComponentId[mergedNodeId] = componentId;
            }
        }
    }

    // We don't count nodes that haven't yet been assigned a component
    oldComponentIdsAffected.erase(ComponentId());

    return oldComponentIdsAffected;
}

void ComponentManager::insertComponentArray(IGraphArray* componentArray)
{
    std::unique_lock<std::mutex> lock(_componentArraysMutex);
    _componentArrays.insert(componentArray);
}

void ComponentManager::eraseComponentArray(IGraphArray* componentArray)
{
    std::unique_lock<std::mutex> lock(_componentArraysMutex);
    _componentArrays.erase(componentArray);
}

void ComponentManager::update(const Graph* graph)
{
    if(_debug) qDebug() << "ComponentManager::update begins" << this;

    std::unique_lock<std::recursive_mutex> lock(_updateMutex);

    std::map<ComponentId, ComponentIdSet> splitComponents;
    ComponentIdSet splitComponentIds;
    std::map<ComponentId, ComponentIdSet> mergedComponents;
    ComponentIdSet mergedComponentIds;
    ComponentIdSet componentIds;

    NodeArray<ComponentId> newNodesComponentId(*graph);
    EdgeArray<ComponentId> newEdgesComponentId(*graph);

    // Search for mergers and splitters
    for(auto nodeId : graph->nodeIds())
    {
        if(nodeIdFiltered(nodeId))
            continue;

        auto oldComponentId = _nodesComponentId[nodeId];

        if(newNodesComponentId[nodeId].isNull() && !oldComponentId.isNull())
        {
            if(u::contains(componentIds, oldComponentId))
            {
                // We have already used this ID so this is a component that has split
                auto newComponentId = generateComponentId();
                componentIds.insert(newComponentId);
                assignConnectedElementsComponentId(graph, nodeId, newComponentId,
                                                   newNodesComponentId, newEdgesComponentId);

                queueGraphComponentUpdate(graph, oldComponentId);
                queueGraphComponentUpdate(graph, newComponentId);

                splitComponents[oldComponentId].insert(oldComponentId);
                splitComponents[oldComponentId].insert(newComponentId);
                splitComponentIds.insert(newComponentId);
            }
            else
            {
                componentIds.insert(oldComponentId);
                auto componentIdsAffected = assignConnectedElementsComponentId(graph, nodeId, oldComponentId,
                                                                               newNodesComponentId, newEdgesComponentId);
                queueGraphComponentUpdate(graph, oldComponentId);

                if(componentIdsAffected.size() > 1)
                {
                    // More than one old component IDs were observed so components have merged
                    mergedComponents[oldComponentId].insert(componentIdsAffected.begin(), componentIdsAffected.end());
                    componentIdsAffected.erase(oldComponentId);
                    mergedComponentIds.insert(componentIdsAffected.begin(), componentIdsAffected.end());
                }
            }
        }
    }

    // Search for entirely new components
    for(auto nodeId : graph->nodeIds())
    {
        if(nodeIdFiltered(nodeId))
            continue;

        if(newNodesComponentId[nodeId].isNull() && _nodesComponentId[nodeId].isNull())
        {
            auto newComponentId = generateComponentId();
            componentIds.insert(newComponentId);
            assignConnectedElementsComponentId(graph, nodeId, newComponentId, newNodesComponentId, newEdgesComponentId);
            queueGraphComponentUpdate(graph, newComponentId);
        }
    }

    // Resize the component arrays
    for(auto* componentArray : _componentArrays)
        componentArray->resize(componentArrayCapacity());

    // Search for added or removed components
    auto componentIdsToBeAdded = u::setDifference(componentIds, _componentIds);
    auto componentIdsToBeRemoved = u::setDifference(_componentIds, componentIds);

    // Find nodes and edges that have been added or removed
    std::map<ComponentId, std::vector<NodeId>> nodeIdAdds;
    std::map<ComponentId, std::vector<EdgeId>> edgeIdAdds;
    std::map<ComponentId, std::vector<NodeId>> nodeIdRemoves;
    std::map<ComponentId, std::vector<EdgeId>> edgeIdRemoves;

    auto maxNumNodes = std::max(_nodesComponentId.size(), newNodesComponentId.size());
    for(NodeId nodeId(0); nodeId < maxNumNodes; ++nodeId)
    {
        if(_nodesComponentId[nodeId].isNull() && !newNodesComponentId[nodeId].isNull())
            nodeIdAdds[newNodesComponentId[nodeId]].emplace_back(nodeId);
        else if(!_nodesComponentId[nodeId].isNull() && newNodesComponentId[nodeId].isNull())
            nodeIdRemoves[_nodesComponentId[nodeId]].emplace_back(nodeId);
    }

    auto maxNumEdges = std::max(_edgesComponentId.size(), newEdgesComponentId.size());
    for(EdgeId edgeId(0); edgeId < maxNumEdges; ++edgeId)
    {
        if(_edgesComponentId[edgeId].isNull() && !newEdgesComponentId[edgeId].isNull())
            edgeIdAdds[newEdgesComponentId[edgeId]].emplace_back(edgeId);
        else if(!_edgesComponentId[edgeId].isNull() && newEdgesComponentId[edgeId].isNull())
            edgeIdRemoves[_edgesComponentId[edgeId]].emplace_back(edgeId);
    }

    // Notify all the merges
    for(auto& mergee : mergedComponents)
    {
        if(_debug) qDebug() << "componentsWillMerge" << mergee.second << "->" << mergee.first;
        emit componentsWillMerge(graph, ComponentMergeSet(std::move(mergee.second), mergee.first));
    }

    // Removed components
    for(auto componentId : componentIdsToBeRemoved)
    {
        Q_ASSERT(!componentId.isNull());
        if(_debug) qDebug() << "componentWillBeRemoved" << componentId;
        bool hasMerged = u::contains(mergedComponentIds, componentId);
        emit componentWillBeRemoved(graph, componentId, hasMerged);

        if(!hasMerged)
        {
            nodeIdRemoves.erase(componentId);
            edgeIdRemoves.erase(componentId);
        }

        u::removeByValue(_componentIds, componentId);
        removeGraphComponent(componentId);
    }

    _nodesComponentId = std::move(newNodesComponentId);
    _edgesComponentId = std::move(newEdgesComponentId);

    updateGraphComponents(graph);

    _updatesRequired.clear();

    for(auto componentId : componentIdsToBeAdded)
        _componentIds.push_back(componentId);

    std::sort(_componentIds.begin(), _componentIds.end(), [](auto a, auto b) { return a < b; });

    lock.unlock();

    // Notify all the new components
    for(auto componentId : componentIdsToBeAdded)
    {
        Q_ASSERT(!componentId.isNull());
        if(_debug) qDebug() << "componentAdded" << componentId;
        bool hasSplit = u::contains(splitComponentIds, componentId);
        emit componentAdded(graph, componentId, hasSplit);

        if(!hasSplit)
        {
            nodeIdAdds.erase(componentId);
            edgeIdAdds.erase(componentId);
        }
    }

    // Notify all the splits
    for(auto& splitee : splitComponents)
    {
        if(_debug) qDebug() << "componentSplit" << splitee.first << "->" << splitee.second;
        emit componentSplit(graph, ComponentSplitSet(splitee.first, std::move(splitee.second)));
    }

    // Notify node adds and removes
    for(auto& nodeIdAdd : nodeIdAdds)
    {
        for(auto nodeId : nodeIdAdd.second)
            emit nodeAddedToComponent(graph, nodeId, nodeIdAdd.first);
    }

    for(auto& edgeIdAdd : edgeIdAdds)
    {
        for(auto edgeId : edgeIdAdd.second)
            emit edgeAddedToComponent(graph, edgeId, edgeIdAdd.first);
    }

    for(auto& nodeIdRemove : nodeIdRemoves)
    {
        for(auto nodeId : nodeIdRemove.second)
            emit nodeRemovedFromComponent(graph, nodeId, nodeIdRemove.first);
    }

    for(auto& edgeIdRemove : edgeIdRemoves)
    {
        for(auto edgeId : edgeIdRemove.second)
            emit edgeRemovedFromComponent(graph, edgeId, edgeIdRemove.first);
    }

    if(_debug) qDebug() << "ComponentManager::update ends" << this;
}

ComponentId ComponentManager::generateComponentId()
{
    ComponentId newComponentId;

    if(!_vacatedComponentIdQueue.empty())
    {
        newComponentId = _vacatedComponentIdQueue.front();
        _vacatedComponentIdQueue.pop();
    }
    else
        newComponentId = _nextComponentId++;

    return newComponentId;
}

void ComponentManager::queueGraphComponentUpdate(const Graph* graph, ComponentId componentId)
{
    _updatesRequired.insert(componentId);

    if(!u::contains(_componentsMap, componentId))
    {
        auto graphComponent = std::make_unique<GraphComponent>(graph);
        _componentsMap.emplace(componentId, std::move(graphComponent));
    }
}

void ComponentManager::updateGraphComponents(const Graph* graph)
{
    for(auto& graphComponent : _componentsMap)
    {
        if(u::contains(_updatesRequired, graphComponent.first))
        {
            graphComponent.second->_nodeIds.clear();
            graphComponent.second->_edgeIds.clear();
        }
    }

    for(auto nodeId : graph->nodeIds())
    {
        if(nodeIdFiltered(nodeId))
            continue;

        auto componentId = _nodesComponentId[nodeId];

        if(u::contains(_updatesRequired, componentId))
            _componentsMap[componentId]->_nodeIds.push_back(nodeId);
    }

    for(auto edgeId : graph->edgeIds())
    {
        if(edgeIdFiltered(edgeId))
            continue;

        auto componentId = _edgesComponentId[edgeId];

        if(u::contains(_updatesRequired, componentId))
            _componentsMap[componentId]->_edgeIds.push_back(edgeId);
    }
}

void ComponentManager::removeGraphComponent(ComponentId componentId)
{
    if(u::contains(_componentsMap, componentId))
    {
        _componentsMap.erase(componentId);
        _vacatedComponentIdQueue.push(componentId);
        _updatesRequired.erase(componentId);
    }
}

void ComponentManager::onGraphChanged(const Graph* graph, bool changeOccurred)
{
    if(changeOccurred)
    {
        graph->setPhase(tr("Componentising"));
        update(graph);
    }
}

#include <chrono>

template<typename T> class unique_lock_with_warning
{
public:
    unique_lock_with_warning() = default;
    unique_lock_with_warning(unique_lock_with_warning&&) noexcept = default;
    unique_lock_with_warning(const unique_lock_with_warning&) = delete;
    unique_lock_with_warning& operator=(unique_lock_with_warning&&) noexcept = default;
    unique_lock_with_warning& operator=(const unique_lock_with_warning&) = delete;

    explicit unique_lock_with_warning(T& mutex) :
        _lock(mutex, std::defer_lock)
    {
        const int MIN_WARNING_MILLISECONDS = 100;
        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

        if(!_lock.try_lock())
        {
            _lock.lock();

            std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
            auto timeToAcquireLock = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            if(timeToAcquireLock > MIN_WARNING_MILLISECONDS)
            {
                qWarning() << "WARNING: thread" << u::currentThreadName() <<
                              "was blocked for" << timeToAcquireLock << "ms";
            }
        }
    }

    ~unique_lock_with_warning()
    {
        if(_lock.owns_lock())
            _lock.unlock();
    }

private:
    std::unique_lock<T> _lock;
};

const std::vector<ComponentId>& ComponentManager::componentIds() const
{
    unique_lock_with_warning<std::recursive_mutex> lock(_updateMutex);

    return _componentIds;
}

bool ComponentManager::containsComponentId(ComponentId componentId) const
{
    unique_lock_with_warning<std::recursive_mutex> lock(_updateMutex);

    return u::contains(_componentsMap, componentId);
}

const GraphComponent* ComponentManager::componentById(ComponentId componentId) const
{
    unique_lock_with_warning<std::recursive_mutex> lock(_updateMutex);

    if(u::contains(_componentsMap, componentId))
        return _componentsMap.at(componentId).get();

    Q_ASSERT(!"ComponentManager::componentById returning nullptr");
    return nullptr;
}

ComponentId ComponentManager::componentIdOfNode(NodeId nodeId) const
{
    if(nodeId.isNull())
        return {};

    unique_lock_with_warning<std::recursive_mutex> lock(_updateMutex);

    auto componentId = _nodesComponentId.at(nodeId);
    auto i = std::find(_componentIds.begin(), _componentIds.end(), componentId);
    if(i != _componentIds.end())
        return *i;

    qDebug() << "Can't find componentId of nodeId" << nodeId;
    return {};
}

ComponentId ComponentManager::componentIdOfEdge(EdgeId edgeId) const
{
    if(edgeId.isNull())
        return ComponentId();

    unique_lock_with_warning<std::recursive_mutex> lock(_updateMutex);

    auto componentId = _edgesComponentId.at(edgeId);
    auto i = std::find(_componentIds.begin(), _componentIds.end(), componentId);
    return i != _componentIds.end() ? *i : ComponentId();
}

