// Em "calculators/DirectStatisticsCalculator.h"

#ifndef DIRECTSTATISTICSCALCULATOR_H_
#define DIRECTSTATISTICSCALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <map>
#include <string>

/**
 * @class DirectStatisticsCalculator
 * @brief Especialista dedicado a calcular as features do tipo "Direct Statistical" (DS).
 */
class DirectStatisticsCalculator {
public:
    DirectStatisticsCalculator() = default;

    /**
     * @brief Orquestra o cálculo de todas as features DS, delegando para métodos especialistas.
     */
    void calculate(const CVRP& problem, FeatureSet& features) const;

private:
/**
     * @brief Calcula as features DS1: Distribution of distance matrix values.
     * @return Retorna o vetor de distâncias para que possa ser reutilizado por outras funções.
     */
    std::vector<double> calculate_ds1_features(const CVRP& problem, FeatureSet& features) const;

    /**
     * @brief Calcula as features DS2: Fraction of Distinct Distances.
     */
    void calculate_ds2_features(const std::vector<double>& all_distances, FeatureSet& features) const;

    /**
     * @brief Calcula a feature DS3: Distance Matrix Symmetry Classification.
     */
    void calculate_ds3_features(const CVRP& problem, FeatureSet& features) const;

    /**
     * @brief Calcula as features DS4: Triangle Inequality Adherence.
     */
    void calculate_ds4_features(const CVRP& problem, FeatureSet& features) const;
};

#endif /* DIRECTSTATISTICSCALCULATOR_H_ */