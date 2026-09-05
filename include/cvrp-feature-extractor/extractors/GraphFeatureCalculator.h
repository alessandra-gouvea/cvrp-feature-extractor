#ifndef GRAPHFEATURECALCULATOR_H_
#define GRAPHFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>

/**
 * @class GraphFeatureCalculator
 * @brief Especialista dedicado a calcular features baseadas na teoria dos grafos.
 *
 * Esta classe constrói uma representação de grafo da instância CVRP e usa
 * algoritmos (como MST) da Boost Graph Library para extrair features estruturais.
 */
class GraphFeatureCalculator {
public:
    GraphFeatureCalculator() = default;

    /**
     * @brief Calcula todas as features GB e as adiciona ao mapa de resultados.
     * @param problem A instância CVRP a ser analisada.
     * @param features O mapa onde as novas features serão inseridas.
     */
    void calculate(const CVRP& problem, FeatureSet& features) const;

private:
    void calculate_gb1_vertex_cost_features(const CVRP& problem, FeatureSet& features) const;
    void calculate_gb2_mbc_features(const CVRP& problem, FeatureSet& features) const;
    void calculate_gb3_mst_features(const CVRP& problem, FeatureSet& features) const;
    void calculate_gb4_1_local_neighborhood_features(const CVRP& problem, FeatureSet& features) const;
    void calculate_gb4_2_dknng_features(const CVRP& problem, FeatureSet& features) const;
};

#endif /* GRAPHFEATURECALCULATOR_H_ */