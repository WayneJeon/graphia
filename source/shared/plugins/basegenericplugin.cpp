#include "basegenericplugin.h"

#include "shared/loading/gmlfileparser.h"
#include "shared/loading/pairwisetxtfileparser.h"
#include "shared/loading/graphmlparser.h"

#include "shared/attributes/iattribute.h"

#include "shared/utils/container.h"
#include "shared/utils/iterator_range.h"

#include "thirdparty/json/json_helper.h"

BaseGenericPluginInstance::BaseGenericPluginInstance()
{
    connect(this, SIGNAL(loadSuccess()), this, SLOT(onLoadSuccess()));
    connect(this, SIGNAL(selectionChanged(const ISelectionManager*)),
            this, SLOT(onSelectionChanged(const ISelectionManager*)), Qt::DirectConnection);
}

void BaseGenericPluginInstance::initialise(const IPlugin* plugin, IDocument* document,
                                           const IParserThread* parserThread)
{
    BasePluginInstance::initialise(plugin, document, parserThread);

    auto graphModel = document->graphModel();
    _userNodeData.initialise(graphModel->mutableGraph());
    _nodeAttributeTableModel.initialise(document, &_userNodeData);
}

std::unique_ptr<IParser> BaseGenericPluginInstance::parserForUrlTypeName(const QString& urlTypeName)
{
    if(urlTypeName == QLatin1String("GML"))
        return std::make_unique<GmlFileParser>(&_userNodeData);
    else if(urlTypeName == QLatin1String("PairwiseTXT"))
        return std::make_unique<PairwiseTxtFileParser>(this, &_userNodeData);
    else if(urlTypeName == QLatin1String("GraphML"))
        return std::make_unique<GraphMLParser>(&_userNodeData);

    return nullptr;
}

void BaseGenericPluginInstance::setEdgeWeight(EdgeId edgeId, float weight)
{
    if(_edgeWeights == nullptr)
    {
        _edgeWeights = std::make_unique<EdgeArray<float>>(graphModel()->mutableGraph());

        graphModel()->createAttribute(tr("Edge Weight"))
            .setFloatValueFn([this](EdgeId edgeId_) { return _edgeWeights->get(edgeId_); })
            .setFlag(AttributeFlag::AutoRangeMutable)
            .setDescription(tr("The Edge Weight is a generic value associated with the edge."))
            .setUserDefined(true);
    }

    _edgeWeights->set(edgeId, weight);
}

QByteArray BaseGenericPluginInstance::save(IMutableGraph& graph, const ProgressFn& progressFn) const
{
    json jsonObject;

    if(_edgeWeights != nullptr)
    {
        graph.setPhase(QObject::tr("Edge Weights"));
        auto range = make_iterator_range(_edgeWeights->cbegin(), _edgeWeights->cbegin() + graph.nextEdgeId());
        jsonObject["edgeWeights"] = jsonArrayFrom(range, progressFn);
    }

    progressFn(-1);

    jsonObject["userNodeData"] = _userNodeData.save(graph, progressFn);

    return QByteArray::fromStdString(jsonObject.dump());
}

bool BaseGenericPluginInstance::load(const QByteArray& data, int dataVersion,
                                     IMutableGraph& graph, const ProgressFn& progressFn)
{
    if(dataVersion != plugin()->dataVersion())
        return false;

    json jsonObject = json::parse(data.begin(), data.end(), nullptr, false);

    if(jsonObject.is_null() || !jsonObject.is_object())
        return false;

    if(u::contains(jsonObject, "edgeWeights") && jsonObject["edgeWeights"].is_array())
    {
        const auto& jsonEdgeWeights = jsonObject["edgeWeights"];

        uint64_t i = 0;

        graph.setPhase(QObject::tr("Edge Weights"));
        for(const auto& edgeWeight : jsonEdgeWeights)
        {
            if(graph.containsEdgeId(i))
                setEdgeWeight(i, edgeWeight);

            progressFn(static_cast<int>((i++ * 100) / jsonEdgeWeights.size()));
        }
    }

    progressFn(-1);

    if(!u::contains(jsonObject, "userNodeData") || !jsonObject["userNodeData"].is_object())
        return false;

    const auto& jsonUserNodeData = jsonObject["userNodeData"];

    if(!_userNodeData.load(jsonUserNodeData, progressFn))
        return false;

    return true;
}

QString BaseGenericPluginInstance::selectedNodeNames() const
{
    QString s;

    for(auto nodeId : selectionManager()->selectedNodes())
    {
        if(!s.isEmpty())
            s += QLatin1String(", ");

        s += graphModel()->nodeName(nodeId);
    }

    return s;
}

void BaseGenericPluginInstance::onLoadSuccess()
{
    _userNodeData.setNodeNamesToFirstUserDataVector(*graphModel());
    _userNodeData.exposeAsAttributes(*graphModel());
    _nodeAttributeTableModel.updateRoleNames();
}

void BaseGenericPluginInstance::onSelectionChanged(const ISelectionManager*)
{
    emit selectedNodeNamesChanged();
    _nodeAttributeTableModel.onSelectionChanged();
}

BaseGenericPlugin::BaseGenericPlugin()
{
    registerUrlType(QStringLiteral("GML"), QObject::tr("GML File"), QObject::tr("GML Files"), {"gml"});
    registerUrlType(QStringLiteral("PairwiseTXT"), QObject::tr("Pairwise Text File"), QObject::tr("Pairwise Text Files"), {"txt", "layout"});
    registerUrlType(QStringLiteral("GraphML"), QObject::tr("GraphML File"), QObject::tr("GraphML Files"), {"graphml"});

}

QStringList BaseGenericPlugin::identifyUrl(const QUrl& url) const
{
    //FIXME actually look at the file contents
    return identifyByExtension(url);
}
