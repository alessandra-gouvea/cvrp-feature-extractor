
#ifndef GEOMETRICFEATURECALCULATOR_H_
#define GEOMETRICFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>

/**
 * @class GeometricFeatureCalculator
 * @brief Especialista dedicado a calcular as features geométricas (GC).
 *
 * Esta classe encapsula todos os cálculos baseados nas coordenadas 2D dos nós,
 * incluindo centroide, bounding box, e casco convexo (usando a biblioteca Boost).
 */
class GeometricFeatureCalculator {
public:
    GeometricFeatureCalculator() = default;

    /**
     * @brief Calcula todas as features GC e as adiciona ao mapa de resultados.
     * @param problem A instância CVRP a ser analisada.
     * @param features O mapa onde as novas features serão inseridas.
     */
    void calculate(const CVRP& problem, FeatureSet& features) const;

private:
    // --- Métodos Privados de Cálculo ---
    // Cada método é responsável por um grupo de features e pode retornar
    // resultados intermediários para serem usados por outros métodos.

    // Calcula GC1 e retorna as coordenadas do centroide para uso posterior.
    std::pair<double, double> calculate_gc1_centroid(const CVRP& problem, FeatureSet& features) const;

    // Calcula GC2 usando o centroide previamente calculado.
    void calculate_gc2_dist_to_centroid(const CVRP& problem, const std::pair<double, double>& centroid, FeatureSet& features) const;
    
    // Calcula GC3.
    void calculate_gc3_bounding_box(const CVRP& problem, FeatureSet& features) const;

    // Calcula GC4 usando a biblioteca Boost.Geometry.
    void calculate_gc4_convex_hull(const CVRP& problem, FeatureSet& features) const;

     /**
     * @brief Calcula as features de distribuição de ângulos (GC5).
     * @param problem A instância CVRP.
     * @param features O mapa para inserir os resultados do grupo GC5.
     */
    void calculate_gc5_AngleFeatures(const CVRP& problem, FeatureSet& features) const;
};

#endif /* GEOMETRICFEATURECALCULATOR_H_ */