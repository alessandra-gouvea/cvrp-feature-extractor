#pragma once
#include <string>
#include <vector>
#include <map>

// DTO (Data Transfer Object) para carregar os dados brutos do arquivo.
struct ProblemData {
    std::string name;
    std::string type;
    std::string comment;
    std::string edge_weight_type;
    std::string edge_weight_format;

    int dimension = 0;
    int capacity = 0;

    // Usaremos vetores para eficiência, acessados por (id - 1).
    std::vector<std::pair<double, double>> node_coords;
    std::vector<int> demands;
    
    // Para matrizes de distância explícitas
    std::vector<double> lower_row_distances; 
    
    // Apenas armazena o ID. A classe CVRP decidirá o que fazer com ele.
    int depot_id = -1; 
};