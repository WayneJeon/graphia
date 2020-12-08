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

#ifndef MATFILEPARSER_H
#define MATFILEPARSER_H

#include "shared/loading/iparser.h"
#include "shared/loading/userelementdata.h"

#include <matio.h>

class MatFileParser : public IParser
{
private:
    UserNodeData* _userNodeData;
    UserEdgeData* _userEdgeData;

public:
    MatFileParser(UserNodeData* userNodeData, UserEdgeData* userEdgeData) :
        _userNodeData(userNodeData), _userEdgeData(userEdgeData)
    {}

    bool parse(const QUrl& url, IGraphModel* graphModel) override;
    static bool canLoad(const QUrl&) { return true; }

    template<class T>
    bool matVarToGraph(const matvar_t& matvar, IGraphModel* graphModel)
    {
        size_t height = matvar.dims[0];
        size_t width = matvar.dims[1];
        auto totalIterations = static_cast<int>((height * width) + height);
        int progress = 0;

        std::map<size_t, NodeId> indexToNodeId;
        auto* dataptr = static_cast<T*>(matvar.data);

        for(size_t row = 0; row < height; ++row)
        {
            if(cancelled())
                return false;

            auto nodeId = graphModel->mutableGraph().addNode();
            indexToNodeId[row] = nodeId;

            _userNodeData->setValueBy(nodeId, QObject::tr("Node Name"), QObject::tr("Node %1").arg(row + 1));
            progress++;
            setProgress(static_cast<int>(progress * 100 / totalIterations));
        }

        for(size_t row = 0; row < height; ++row)
        {
            for(size_t column = 0; column < width; ++column)
            {
                if(cancelled())
                    return false;

                T value = *(dataptr + (column * matvar.dims[0]) + row);

                auto edgeId = graphModel->mutableGraph().addEdge(indexToNodeId[row], indexToNodeId[column]);
                _userEdgeData->setValueBy(edgeId, QObject::tr("Edge Weight"), QString::number(value));
                progress++;

                setProgress(static_cast<int>(progress * 100 / totalIterations));
            }
        }

        return true;
    }

    bool processMatVarData(const matvar_t& matvar, IGraphModel* graphModel)
    {
        switch(matvar.data_type)
        {
        case MAT_T_DOUBLE: return matVarToGraph<double>(matvar, graphModel);
        case MAT_T_SINGLE: return matVarToGraph<float>(matvar, graphModel);
        case MAT_T_INT64: return matVarToGraph<mat_int64_t>(matvar, graphModel);
        case MAT_T_UINT64: return matVarToGraph<mat_uint64_t>(matvar, graphModel);
        case MAT_T_INT32: return matVarToGraph<mat_int32_t>(matvar, graphModel);
        case MAT_T_UINT32: return matVarToGraph<mat_uint32_t>(matvar, graphModel);
        case MAT_T_INT16: return matVarToGraph<mat_int16_t>(matvar, graphModel);
        case MAT_T_UINT16: return matVarToGraph<mat_uint16_t>(matvar, graphModel);
        case MAT_T_INT8: return matVarToGraph<mat_int8_t>(matvar, graphModel);
        case MAT_T_UINT8: return matVarToGraph<mat_uint8_t>(matvar, graphModel);
        default: return false;
        }
    }
};

#endif // MATFILEPARSER_H
