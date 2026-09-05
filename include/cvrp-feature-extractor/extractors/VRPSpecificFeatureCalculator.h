#ifndef VRPSPECIFICFEATURECALCULATOR_H_
#define VRPSPECIFICFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <cvrp-feature-extractor/core/SharedTypes.h>

/**
 * @class VRPSpecificFeatureCalculator
 * @brief Especialista dedicado a calcular features de "apertatez" das restrições (CT).
 *
 * Mede características como a relação entre a demanda dos clientes e a capacidade
 * dos veículos, que são cruciais para a dificuldade de instâncias CVRP.
 */
class VRPSpecificFeatureCalculator {
public:
    VRPSpecificFeatureCalculator() = default;

    /**
     * @brief Calcula todas as features CT e as adiciona ao mapa de resultados.
     * @param problem A instância CVRP a ser analisada.
     * @param features O mapa onde as novas features serão inseridas.
     */
    void calculate(const CVRP& problem, const ClusteringResult& clustering_result, FeatureSet& features) const;
};

#endif /* VRPSPECIFICFEATURECALCULATOR_H_ */