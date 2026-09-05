
#ifndef GEOMETRICFEATURECALCULATOR_H_
#define GEOMETRICFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_point.hpp>

using point_2d = boost::geometry::model::d2::point_xy<double>;
using polygon_2d = boost::geometry::model::polygon<point_2d>;
using multi_point_2d = boost::geometry::model::multi_point<point_2d>;

struct GeometricContext {
    double bb_width = 0.0;
    double bb_height = 0.0;
};

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
    GeometricContext calculate(const CVRP& problem, FeatureSet& features) const;

private:
    // Calcula GC1 e retorna as coordenadas do centroide para uso posterior.
    std::pair<double, double> calculate_gc1_centroid(const CVRP& problem, FeatureSet& features) const;
    // Calcula GC2 usando o centroide previamente calculado.
    void calculate_gc2_dist_to_centroid(const CVRP& problem, const std::pair<double, double>& centroid, FeatureSet& features) const;
    GeometricContext calculate_gc3_bounding_box(const CVRP& problem, FeatureSet& features) const;
    // Calcula GC4 usando a biblioteca Boost.Geometry.
    polygon_2d calculate_gc4_convex_hull(const CVRP& problem, FeatureSet& features) const;
    void calculate_gc5_AngleFeatures(const CVRP& problem, FeatureSet& features) const;
    void calculate_gc6_CosineAngleFeatures(const CVRP& problem, FeatureSet& features) const; 
    void calculate_gc7_voronoi(const CVRP& problem, const polygon_2d& hull, FeatureSet& features) const;
    void calculate_gc8_delaunay(const CVRP& problem, const polygon_2d& hull, FeatureSet& features) const;
};

#endif /* GEOMETRICFEATURECALCULATOR_H_ */