#ifndef SHAREDTYPES_H_
#define SHAREDTYPES_H_

#include <vector>
#include <map>
#include <cstddef> // Para size_t
#include <limits>  // Para numeric_limits

// Estrutura para manter os resultados brutos de uma execução do DBSCAN.
struct ClusteringResult {
    // Mapeia o ID de um cluster para os índices dos pontos que pertencem a ele.
    std::map<size_t, std::vector<size_t>> points_in_cluster;
    
    // Lista dos índices dos pontos classificados como outliers.
    std::vector<size_t> outlier_indices;

    // Constante para identificar outliers nos resultados brutos da mlpack.
    static constexpr size_t OUTLIER_ID = std::numeric_limits<size_t>::max();
};

#endif /* SHAREDTYPES_H_ */