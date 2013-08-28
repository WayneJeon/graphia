#include "simplecomponentmanager.h"

#include <QQueue>

void SimpleComponentManager::assignConnectedElementsComponentId(NodeId rootId, ComponentId componentId, EdgeId skipEdgeId)
{
    QQueue<NodeId> nodeIdSearchList;

    nodeIdSearchList.enqueue(rootId);

    while(!nodeIdSearchList.isEmpty())
    {
        NodeId nodeId = nodeIdSearchList.dequeue();
        nodesComponentId[nodeId] = componentId;

        const QSet<EdgeId> edgeIds = graph().nodeById(nodeId).edges();

        for(EdgeId edgeId : edgeIds)
        {
            if(edgeId == skipEdgeId)
                continue;

            edgesComponentId[edgeId] = componentId;
            NodeId oppositeNodeId = graph().edgeById(edgeId).oppositeId(nodeId);

            if(nodesComponentId[oppositeNodeId] != componentId)
            {
                nodeIdSearchList.enqueue(oppositeNodeId);
                nodesComponentId[oppositeNodeId] = componentId;
            }
        }
    }
}

ComponentId SimpleComponentManager::generateComponentId()
{
    ComponentId newComponentId;

    if(!vacatedComponentIdQueue.isEmpty())
        newComponentId = vacatedComponentIdQueue.dequeue();
    else
        newComponentId = nextComponentId++;

    componentIdsList.append(newComponentId);

    return newComponentId;
}

void SimpleComponentManager::releaseComponentId(ComponentId componentId)
{
    componentIdsList.removeOne(componentId);
    vacatedComponentIdQueue.enqueue(componentId);
}

void SimpleComponentManager::updateGraphComponent(ComponentId componentId)
{
    GraphComponent* graphComponent;

    if(!componentsMap.contains(componentId))
    {
        graphComponent = new GraphComponent(this->graph());
        componentsMap.insert(componentId, graphComponent);
        updatesRequired.insert(componentId);
    }

    if(!updatesRequired.contains(componentId))
        return;

    updatesRequired.remove(componentId);
    graphComponent = componentsMap[componentId];

    QList<NodeId>& nodeIdsList = graphComponentNodeIdsList(graphComponent);
    QList<NodeId>& edgeIdsList = graphComponentEdgeIdsList(graphComponent);

    nodeIdsList.clear();
    const QList<NodeId>& nodeIds = graph().nodeIds();
    for(NodeId nodeId : nodeIds)
    {
        if(nodesComponentId[nodeId] == componentId)
            nodeIdsList.append(nodeId);
    }

    edgeIdsList.clear();
    const QList<NodeId>& edgeIds = graph().edgeIds();
    for(NodeId edgeId : edgeIds)
    {
        if(edgesComponentId[edgeId] == componentId)
            edgeIdsList.append(edgeId);
    }
}

void SimpleComponentManager::removeGraphComponent(ComponentId componentId)
{
    if(componentsMap.contains(componentId))
    {
        delete componentsMap[componentId];
        componentsMap.remove(componentId);
    }
}

void SimpleComponentManager::nodeAdded(NodeId nodeId)
{
    ComponentId newComponentId = generateComponentId();
    nodesComponentId[nodeId] = newComponentId;

    // New component
    emit componentAdded(&graph(), newComponentId);
}

void SimpleComponentManager::nodeWillBeRemoved(NodeId nodeId)
{
    if(graph().nodeById(nodeId).degree() == 0)
    {
        ComponentId componentId = nodesComponentId[nodeId];
        removeGraphComponent(componentId);

        // Component removed
        emit componentRemoved(&graph(), componentId);
    }
}

void SimpleComponentManager::edgeAdded(EdgeId edgeId)
{
    const Edge& edge = graph().edgeById(edgeId);

    if (nodesComponentId[edge.sourceId()] != nodesComponentId[edge.targetId()])
    {
        ComponentId firstComponentId = nodesComponentId[edge.sourceId()];
        ComponentId secondComponentId = nodesComponentId[edge.targetId()];

        // Assign every node in the second component to the first
        assignConnectedElementsComponentId(edge.targetId(), firstComponentId, edgeId);
        edgesComponentId[edgeId] = firstComponentId;
        updatesRequired.insert(firstComponentId);
        releaseComponentId(secondComponentId);
        removeGraphComponent(secondComponentId);

        // Components merged
        emit componentsMerged(&graph(), firstComponentId, {firstComponentId, secondComponentId});

        // Component removed
        emit componentRemoved(&graph(), secondComponentId);
    }
}

void SimpleComponentManager::edgeWillBeRemoved(EdgeId edgeId)
{
    ComponentId newComponentId = generateComponentId();
    const Edge& edge = graph().edgeById(edgeId);
    ComponentId oldComponentId = nodesComponentId[edge.sourceId()];

    // Assign every node connected to the target of the removed edge the new componentId
    assignConnectedElementsComponentId(edge.targetId(), newComponentId, edgeId);
    updatesRequired.insert(oldComponentId);

    if(nodesComponentId[edge.sourceId()] == nodesComponentId[edge.targetId()])
    {
        // The edge removal didn't create a new component,
        // so go back to using the original ComponentId
        const QList<NodeId>& nodeIds = graph().nodeIds();
        for(NodeId nodeId : nodeIds)
            nodesComponentId[nodeId] = oldComponentId;

        releaseComponentId(newComponentId);
    }
    else
    {
        // Components split
        emit componentSplit(&graph(), oldComponentId, {oldComponentId, newComponentId});

        // New component
        emit componentAdded(&graph(), newComponentId);
    }
}

const QList<ComponentId>& SimpleComponentManager::componentIds() const
{
    return componentIdsList;
}

const ReadOnlyGraph& SimpleComponentManager::componentById(ComponentId componentId)
{
    updateGraphComponent(componentId);
    return *componentsMap[componentId];
}

