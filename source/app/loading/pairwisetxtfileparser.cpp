#include "pairwisetxtfileparser.h"

#include "../utils/utils.h"
#include "../graph/mutablegraph.h"
#include "../graph/weightededgegraphmodel.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QStringList>

#include <QDebug>

#include <unordered_map>
#include <vector>

#include <string>
#include <iostream>
#include <fstream>
#include <cctype>

bool PairwiseTxtFileParser::parse(MutableGraph& graph)
{
    std::ifstream file(_filename.toStdString());
    if(!file)
        return false;

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::end);
    fileSize = file.tellg() - fileSize;

    std::unordered_map<std::string, NodeId> nodeIdHash;

    int percentComplete = 0;
    std::string line;
    std::string token;
    std::vector<std::string> tokens;

    file.seekg(0, std::ios::beg);
    while(std::getline(file, line))
    {
        if(cancelled())
            return false;

        tokens.clear();

        bool inQuotes = false;

        for(size_t i = 0; i < line.length(); i++)
        {
            if((i + 1) < line.length() &&
               line[i] == '/' && line[i + 1] == '/')
            {
                // Ignore C++ style comments
                break;
            }
            else if(line[i] == '\"')
            {
                if(inQuotes)
                {
                    tokens.emplace_back(std::move(token));
                    token.clear();
                }

                inQuotes = !inQuotes;
            }
            else
            {
                bool space = std::isspace(line[i]);
                bool trailingSpace = space && !token.empty();

                if(trailingSpace && !inQuotes)
                {
                    tokens.emplace_back(std::move(token));
                    token.clear();
                }
                else if(!space || inQuotes)
                    token += line[i];
            }
        }

        if(!token.empty())
        {
            tokens.emplace_back(std::move(token));
            token.clear();
        }

        if(tokens.size() >= 2)
        {
            auto& firstToken = tokens.at(0);
            auto& secondToken = tokens.at(1);

            NodeId firstNodeId;
            NodeId secondNodeId;

            if(!u::contains(nodeIdHash, firstToken))
            {
                firstNodeId = graph.addNode();
                nodeIdHash.emplace(firstToken, firstNodeId);
                _graphModel->setNodeName(firstNodeId, QString::fromStdString(firstToken));
            }
            else
                firstNodeId = nodeIdHash[firstToken];

            if(!u::contains(nodeIdHash, secondToken))
            {
                secondNodeId = graph.addNode();
                nodeIdHash.emplace(secondToken, secondNodeId);
                _graphModel->setNodeName(secondNodeId, QString::fromStdString(secondToken));
            }
            else
                secondNodeId = nodeIdHash[secondToken];

            auto edgeId = graph.addEdge(firstNodeId, secondNodeId);

            if(tokens.size() >= 3)
            {
                // We have an edge weight too
                auto& thirdToken = tokens.at(2);
                _graphModel->setEdgeWeight(edgeId, std::atof(thirdToken.c_str()));
            }
        }

        int newPercentComplete = static_cast<int>(file.tellg() * 100 / fileSize);

        if(newPercentComplete > percentComplete)
        {
            percentComplete = newPercentComplete;
            emit progress(newPercentComplete);
        }
    }

    return true;
}
