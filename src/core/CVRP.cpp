/*
 * CVRP.h
 *
 *  Created on: 26 de abr de 2024
 *      Author: Alessandra M M M Gouvea
 */

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/algorithms/MultidimensionalScaling.h>

CVRP::CVRP(const ProblemData& data) {
    initializeFromData(data);
}

void CVRP::initializeFromData(const ProblemData& data) {
    // Passo 1: Copiar os dados simples
    this->name = data.name;
    this->dimension = data.dimension;
    this->capacity = data.capacity;

    if (this->dimension <= 0) {
        throw std::runtime_error("CVRP Initialization Error: Dimension must be positive.");
    }

    // Passo 2: Criar os objetos Node.
    // Decidimos como criá-los com base nos dados que recebemos.
    if (!data.node_coords.empty()) {
        createNodesFromCoords(data);
    } else if (data.edge_weight_type == "EXPLICIT" && !data.lower_row_distances.empty()) {
        createNodesFromDistances(data);
    } else {
        throw std::runtime_error("CVRP Initialization Error: Cannot create nodes. No coordinates or explicit distance matrix provided.");
    }
    
    // Passo 3: Atribuir as demandas aos nós que acabamos de criar.
    assignDemands(data);
    
    // Passo 4: Configurar o depósito.
    setupDepot(data);
}

void CVRP::createNodesFromCoords(const ProblemData& data) {
    for (int i = 0; i < this->dimension; ++i) {
        int nodeId = i + 1;
        const auto& coords = data.node_coords[i];
        // Usamos std::make_unique para criar o nó de forma segura
        this->nodes[nodeId] = std::make_unique<Node>(nodeId, coords.first, coords.second);
    }
}

void CVRP::createNodesFromDistances(const ProblemData& data) {
    MultidimensionalScaling mds;
    std::vector<std::pair<double, double>> calculated_coords = mds.calculateCoordinates(data.lower_row_distances, this->dimension);

    for (int i = 0; i < this->dimension; ++i) {
        int nodeId = i + 1;
        const auto& coords = calculated_coords[i];
        this->nodes[nodeId] = std::make_unique<Node>(nodeId, coords.first, coords.second);
    }
}

void CVRP::assignDemands(const ProblemData& data) {
    if (data.demands.size() != this->dimension) {
        // Lançar um aviso ou um erro
        return;
    }

    for (int i = 0; i < this->dimension; ++i) {
        int nodeId = i + 1;
        this->nodes[nodeId]->setDemand(data.demands[i]); // Supondo que Node tenha este método
    }
}

void CVRP::setupDepot(const ProblemData& data) {
    if (data.depot_id <= 0 || data.depot_id > this->dimension) {
        throw std::runtime_error("CVRP Initialization Error: Invalid depot ID.");
    }
    
    auto it = this->nodes.find(data.depot_id);
    if (it == this->nodes.end()) {
         throw std::runtime_error("CVRP Internal Error: Depot node object not found after creation.");
    }

    // Configura o ponteiro de acesso rápido.
    this->depot = it->second.get(); // .get() retorna o ponteiro bruto de dentro do unique_ptr.
    this->depot->setAsDepot();     // Supondo que Node tenha este método.
}

// Implementação do getter
const Node* CVRP::getNode(int nodeId) const {
    auto it = this->nodes.find(nodeId);
    if (it != this->nodes.end()) {
        return it->second.get(); // Retorna o ponteiro bruto
    }
    return nullptr; // Retorna nullptr se o ID não existir
}