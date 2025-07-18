/*
 * ProblemFile.cpp
 *
 *  Created on: 25 de jul de 2024
 *      Author: lemar
 */

#include <cvrp-feature-extractor/io/CVRParser.h>

std::pair<std::string, std::string> CVRParser::readKeyValue(const std::string& line) const {
    std::string key, value;
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        // Remove espaços em branco no início e no fim (trim)
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
    }
    return {key, value}; // Usando a sintaxe moderna para criar um par
}

// Function to read the problem file
ProblemData CVRParser::loadProblemFile(const std::string& filename) const { 
    std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + filename);
	}

	ProblemData data;
    std::string line;
    std::string currentSection = "";
    int lineNumber = 0;

	while (getline(file, line)) {
        lineNumber++;
        
        // Limpa a linha
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty() || line == "EOF") continue;

        // Seção de Especificação (chave: valor)
        if (line.find(':') != std::string::npos) {
            auto keyValue = readKeyValue(line);
            try {
                if (keyValue.first == "NAME") data.name = keyValue.second;
                else if (keyValue.first == "TYPE") data.type = keyValue.second;
                else if (keyValue.first == "COMMENT") data.comment = keyValue.second;
                else if (keyValue.first == "EDGE_WEIGHT_TYPE") data.edge_weight_type = keyValue.second;
                else if (keyValue.first == "EDGE_WEIGHT_FORMAT") data.edge_weight_format = keyValue.second;
                else if (keyValue.first == "CAPACITY") data.capacity = std::stoi(keyValue.second);
                else if (keyValue.first == "DIMENSION") data.dimension = std::stoi(keyValue.second);
                // Ignora campos não essenciais ou não reconhecidos
            } catch (const std::exception& e) {
                // Captura erros de conversão (ex: stoi) e lança um erro mais informativo.
                throw std::runtime_error("Invalid format in " + filename + " at line " + std::to_string(lineNumber) + ". Key: " + keyValue.first);
            }
        }
        // Identificação de uma nova seção de dados
        else if (line.find("_SECTION") != std::string::npos) {
            currentSection = line;
            // Prepara os vetores agora que a dimensão é conhecida.
            if (data.dimension > 0) {
                if (currentSection == "NODE_COORD_SECTION" && data.node_coords.empty()) {
                    data.node_coords.resize(data.dimension);
                }
                if (currentSection == "DEMAND_SECTION" && data.demands.empty()) {
                    data.demands.resize(data.dimension);
                }
            }
        }
        // Processamento de dados dentro de uma seção
        else if (!currentSection.empty()) {
             std::stringstream ss(line);
             if (currentSection == "NODE_COORD_SECTION") {
                 if (data.dimension == 0) throw std::runtime_error("DIMENSION must be declared before NODE_COORD_SECTION.");
                 int id;
                 double x, y;
                 ss >> id >> x >> y;
                 if (id > 0 && id <= data.dimension) {
                     data.node_coords[id - 1] = {x, y};
                 }
             } else if (currentSection == "DEMAND_SECTION") {
                 if (data.dimension == 0) throw std::runtime_error("DIMENSION must be declared before DEMAND_SECTION.");
                 int id, demand;
                 ss >> id >> demand;
                 if (id > 0 && id <= data.dimension) {
                     data.demands[id - 1] = demand;
                 }
             } else if (currentSection == "DEPOT_SECTION") {
                 int id;
                 ss >> id;
                 if (id != -1) {
                     data.depot_id = id;
                 }
             } else if (currentSection == "EDGE_WEIGHT_SECTION") {
                 double value;
                 while(ss >> value) {
                     data.lower_row_distances.push_back(value);
                 }
             }
        }
    }

    return data; // Retorna a estrutura de dados preenchida.
}