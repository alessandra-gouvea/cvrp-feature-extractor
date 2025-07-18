/*
 * CVRP.h
 *
 *  Created on: 26 de abr de 2024
 *      Author: Alessandra M M M Gouvea
 */

#ifndef CVRP_H
#define CVRP_H

#include <cvrp-feature-extractor/core/Node.h>
#include <cvrp-feature-extractor/io/ProblemData.h>
#include <unordered_map>
#include <memory>

class CVRP {
public:
    explicit CVRP(const ProblemData& data);
const std::string& getName() const { return name; }
    int getDimension() const { return dimension; }
    int getCapacity() const { return capacity; }
    const Node* getDepot() const { return depot; } // Retorna um ponteiro para o nó do depósito
    const Node* getNode(int nodeId) const;
    const std::map<int, std::unique_ptr<Node>>& getAllNodes() const { return nodes; }
private:
    void initializeFromData(const ProblemData& data);
    void createNodesFromCoords(const ProblemData& data);
    void createNodesFromDistances(const ProblemData& data);
    void assignDemands(const ProblemData& data);
    void setupDepot(const ProblemData& data);

    std::string name;
    int dimension = 0;
    int capacity = 0;
    std::map<int, std::unique_ptr<Node>> nodes;
    Node* depot = nullptr; 
    
};



#endif // CVRP_H