// Em "MultidimensionalScaling.h"

#ifndef MULTIDIMENSIONALSCALING_H_
#define MULTIDIMENSIONALSCALING_H_

#include <vector>
#include <utility> // Para std::pair
#include <string>  // Para std::string em exceções

/**
 * Esta classe é uma "calculadora" especializada que recebe uma matriz de distâncias
 * e a utiliza para gerar um conjunto de coordenadas (geralmente 2D) que melhor
 * aproxima essas distâncias. Ela é completamente independente e não tem estado.
 */
class MultidimensionalScaling {
public:
    MultidimensionalScaling() = default;

    std::vector<std::pair<double, double>> calculateCoordinates(
        const std::vector<double>& lower_row_distances, 
        int dimension) const;
};

#endif /* MULTIDIMENSIONALSCALING_H_ */