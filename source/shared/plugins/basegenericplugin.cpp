/* Copyright © 2013-2020 Graphia Technologies Ltd.
 *
 * This file is part of Graphia.
 *
 * Graphia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Graphia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Graphia.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "basegenericplugin.h"

#include "shared/loading/biopaxfileparser.h"
#include "shared/loading/gmlfileparser.h"
#include "shared/loading/pairwisetxtfileparser.h"
#include "shared/loading/graphmlparser.h"
#include "shared/loading/adjacencymatrixfileparser.h"
#include "shared/loading/jsongraphparser.h"

#include "shared/attributes/iattribute.h"

#include "shared/utils/container.h"

#include <json_helper.h>

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

    auto* graphModel = document->graphModel();

    _userNodeData.initialise(graphModel->mutableGraph());
    _nodeAttributeTableModel.initialise(document, &_userNodeData);

    _userEdgeData.initialise(graphModel->mutableGraph());
}

std::unique_ptr<IParser> BaseGenericPluginInstance::parserForUrlTypeName(const QString& urlTypeName)
{
    if(urlTypeName == QLatin1String("GML"))
        return std::make_unique<GmlFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("PairwiseTXT"))
        return std::make_unique<PairwiseTxtFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("GraphML"))
        return std::make_unique<GraphMLParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("MatrixCSV"))
        return std::make_unique<AdjacencyMatrixCSVFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("MatrixSSV"))
        return std::make_unique<AdjacencyMatrixSSVFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("MatrixTSV"))
        return std::make_unique<AdjacencyMatrixTSVFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("MatrixXLSX"))
        return std::make_unique<AdjacencyMatrixXLSXFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("BiopaxOWL"))
        return std::make_unique<BiopaxFileParser>(&_userNodeData);
    
    if(urlTypeName == QLatin1String("MatrixMatLab"))
        return std::make_unique<AdjacencyMatrixMatLabFileParser>(&_userNodeData, &_userEdgeData);

    if(urlTypeName == QLatin1String("JSONGraph"))
        return std::make_unique<JsonGraphParser>(&_userNodeData, &_userEdgeData);

    return nullptr;
}

QByteArray BaseGenericPluginInstance::save(IMutableGraph& graph, Progressable& progressable) const
{
    json jsonObject;

    progressable.setProgress(-1);

    jsonObject["userNodeData"] = _userNodeData.save(graph, graph.nodeIds(), progressable);
    jsonObject["userEdgeData"] = _userEdgeData.save(graph, graph.edgeIds(), progressable);

    return QByteArray::fromStdString(jsonObject.dump());
}

bool BaseGenericPluginInstance::load(const QByteArray& data, int /*dataVersion*/,
                                     IMutableGraph& graph, IParser& parser)
{
    json jsonObject = parseJsonFrom(data, &parser);

    if(parser.cancelled())
        return false;

    if(jsonObject.is_null() || !jsonObject.is_object())
        return false;

    parser.setProgress(-1);

    if(!u::contains(jsonObject, "userNodeData") || !jsonObject["userNodeData"].is_object())
        return false;

    graph.setPhase(QObject::tr("Node Data"));
    if(!_userNodeData.load(jsonObject["userNodeData"], parser))
        return false;

    if(!u::contains(jsonObject, "userEdgeData") || !jsonObject["userEdgeData"].is_object())
        return false;

    graph.setPhase(QObject::tr("Edge Data"));
    if(!_userEdgeData.load(jsonObject["userEdgeData"], parser))
        return false; // NOLINT

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

void BaseGenericPluginInstance::setHighlightedRows(const QVector<int>& highlightedRows)
{
    if(_highlightedRows.isEmpty() && highlightedRows.isEmpty())
        return;

    _highlightedRows = highlightedRows;

    NodeIdSet highlightedNodeIds;
    for(auto row : highlightedRows)
    {
        auto nodeId = _userNodeData.elementIdForIndex(static_cast<size_t>(row));
        highlightedNodeIds.insert(nodeId);
    }

    document()->highlightNodes(highlightedNodeIds);

    emit highlightedRowsChanged();
}

void BaseGenericPluginInstance::onLoadSuccess()
{
    _userNodeData.exposeAsAttributes(*graphModel());
    _userEdgeData.exposeAsAttributes(*graphModel());
    _nodeAttributeTableModel.updateColumnNames();
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
    registerUrlType(QStringLiteral("MatrixCSV"), QObject::tr("Adjacency Matrix CSV File"), QObject::tr("Adjacency Matrix CSV Files"), {"csv", "matrix"});
    registerUrlType(QStringLiteral("MatrixSSV"), QObject::tr("Adjacency Matrix SSV File"), QObject::tr("Adjacency Matrix SSV Files"), {"csv", "matrix"});
    registerUrlType(QStringLiteral("MatrixTSV"), QObject::tr("Adjacency Matrix File"), QObject::tr("Adjacency Matrix Files"), {"tsv", "matrix"});
    registerUrlType(QStringLiteral("MatrixXLSX"), QObject::tr("Adjacency Matrix Excel File"), QObject::tr("Adjacency Matrix Excel Files"), {"xlsx", "matrix"});
    registerUrlType(QStringLiteral("BiopaxOWL"), QObject::tr("Biopax OWL File"), QObject::tr("Biopax OWL Files"), {"owl"});
    registerUrlType(QStringLiteral("MatrixMatLab"), QObject::tr("Matlab Data File"), QObject::tr("Matlab Data Files"), {"mat"});
    registerUrlType(QStringLiteral("JSONGraph"), QObject::tr("JSON Graph File"), QObject::tr("JSON Graph Files"), {"json"});
}

QStringList BaseGenericPlugin::identifyUrl(const QUrl& url) const
{
    auto urlTypes = identifyByExtension(url);

    if(urlTypes.isEmpty())
        return {};

    QStringList result;

    for(const auto& urlType : urlTypes)
    {
        bool canLoad =
            (urlType == QStringLiteral("GML") && GmlFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("PairwiseTXT") && PairwiseTxtFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("GraphML") && GraphMLParser::canLoad(url)) ||
            (urlType == QStringLiteral("MatrixCSV") && AdjacencyMatrixCSVFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("MatrixSSV") && AdjacencyMatrixSSVFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("MatrixTSV") && AdjacencyMatrixTSVFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("MatrixXLSX") && AdjacencyMatrixXLSXFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("MatrixMatLab") && AdjacencyMatrixMatLabFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("BiopaxOWL") && BiopaxFileParser::canLoad(url)) ||
            (urlType == QStringLiteral("JSONGraph") && JsonGraphParser::canLoad(url));

        if(canLoad)
            result.push_back(urlType);
    }

    return result;
}

QString BaseGenericPlugin::failureReason(const QUrl& url) const
{
    auto urlTypes = identifyByExtension(url);
    if(!urlTypes.isEmpty())
    {
        return tr("The file's contents do not match its filename extension. Extension: %1")
            .arg(urlTypes.join(','));
    }

    return {};
}
