#ifndef FEATUREEXTRACTOR_H_
#define FEATUREEXTRACTOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <map>
#include <string>
#include <vector>

/**
 * @struct NormalizationContext
 * @brief Armazena todos os dados brutos de uma instância que são necessários
 *        para a normalização teórica, mas que não são features em si.
 */
struct NormalizationContext {
    int n = 0;                  // Número de nós (dimensão)
    double bb_width = 0.0;      // Largura do Bounding Box
    double bb_height = 0.0;     // Altura do Bounding Box
    
    // Mapeia um prefixo de feature (ex: "GB4.2_k3") para o valor de k usado.
    // Isso torna a normalização das features k-NNG flexível.
    std::map<std::string, int> k_values; 
};

/**
 * @struct ExtractionResult
 * @brief Encapsula todos os resultados da extração de features.
 */
struct ExtractionResult {
    FeatureSet features;
    NormalizationContext context;
};

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

    ExtractionResult extractFeatures(const CVRP& problem) const;

private:

};

#endif /* FEATUREEXTRACTOR_H_ */