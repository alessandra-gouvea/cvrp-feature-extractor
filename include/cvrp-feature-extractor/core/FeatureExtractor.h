#ifndef FEATUREEXTRACTOR_H_
#define FEATUREEXTRACTOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <map>
#include <string>
#include <vector>

/**
 * @class FeatureExtractor
 * @brief Orquestra o cálculo de um conjunto de features para uma instância CVRP.
 *
 * Esta classe atua como um "Gerente". Ela não calcula nenhuma feature diretamente.
 * Em vez disso, ela delega o trabalho para uma série de classes "Especialistas"
 * (Calculators), cada uma responsável por um grupo específico de features.
 * O resultado de todos os especialistas é consolidado em um único mapa.
 */
class FeatureExtractor {
public:
    FeatureExtractor() = default;

    FeatureSet extractFeatures(const CVRP& problem) const;

private:

};

#endif /* FEATUREEXTRACTOR_H_ */