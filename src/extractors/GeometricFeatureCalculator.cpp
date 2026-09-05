
#include <cvrp-feature-extractor/extractors/GeometricFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>
#include <cmath>
#include <vector>
#include <limits>
#include <numeric>   // Para std::accumulate
#include <algorithm> // Para std::min/max

// ===================================================================================
// INCLUDES E CONFIGURAÇÃO PARA BOOST
// ===================================================================================
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
// Inclui o cabeçalho principal da biblioteca de Voronoi que encontramos
#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/point_data.hpp>

// "Traits" para a classe Node do nosso projeto.
// Isto é um passo de configuração crucial que ensina a Boost.Polygon a "ler"
// nossa classe Node. Colocamos dentro do namespace da boost, como a biblioteca espera.
namespace boost {
namespace polygon {

// 1. Diga à biblioteca que 'Node' se comporta como um ponto.
template <>
struct geometry_concept<Node> {
    typedef point_concept type;
};

// 2. Diga à biblioteca como obter as coordenadas X e Y de um 'Node'.
template <>
struct point_traits<Node> {
    typedef int coordinate_type;
    static inline coordinate_type get(const Node& node, orientation_2d orient) {
        // CORREÇÃO: Adiciona um fator de escala para manter a precisão ao converter para int
        const double scale_factor = 1000.0;
        if (orient == HORIZONTAL) {
            return static_cast<coordinate_type>(std::round(node.getX() * scale_factor));
        }
        return static_cast<coordinate_type>(std::round(node.getY() * scale_factor));
    }
};

} // namespace polygon
} // namespace boost

namespace {
    // --- Função auxiliar para evitar duplicação de código entre GC6 e GC7 ---
    void build_voronoi_diagram(const std::vector<Node>& node_objects, 
        boost::polygon::voronoi_diagram<double>* diagram) {
        construct_voronoi(node_objects.begin(), node_objects.end(), diagram);
    }

    // Função para calcular a área de um triângulo dados 3 nós.
    double triangle_area(const Node* p1, const Node* p2, const Node* p3) {
        if (!p1 || !p2 || !p3) return 0.0;
        
        double val = p1->getX() * (p2->getY() - p3->getY()) +
                    p2->getX() * (p3->getY() - p1->getY()) +
                    p3->getX() * (p1->getY() - p2->getY());
                    
        return std::abs(val) / 2.0;
    }

    // Fórmula de Shoelace para calcular área a partir de uma lista de vértices de double
    double shoelace_formula(const std::vector<point_2d>& vertices) {
        if (vertices.size() < 3) return 0.0;
        double area = 0.0;
        for (size_t i = 0; i < vertices.size(); ++i) {
            const point_2d& p1 = vertices[i];
            const point_2d& p2 = vertices[(i + 1) % vertices.size()];
            area += (p1.x() * p2.y() - p2.x() * p1.y());
        }
        return std::abs(area) / 2.0;
    }
}

// ===================================================================================
// Implementação da Classe
// ===================================================================================

// --- Orquestrador Principal ---
// Função de alto nível que chama os métodos privados em ordem
GeometricContext GeometricFeatureCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() == 0) return{};

    // GC1 e GC2 estão ligados, pois GC2 precisa do resultado de GC1
    const auto centroid = calculate_gc1_centroid(problem, features);
    calculate_gc2_dist_to_centroid(problem, centroid, features);
    GeometricContext geo_context = calculate_gc3_bounding_box(problem, features);
    // O casco é necessário para GC4, GC7 e GC8
    const polygon_2d hull = calculate_gc4_convex_hull(problem, features);
    calculate_gc5_AngleFeatures(problem, features);
    calculate_gc6_CosineAngleFeatures(problem, features); 
    calculate_gc7_voronoi(problem, hull, features);
    calculate_gc8_delaunay(problem, hull, features);

    return geo_context; 
}

// --- Métodos de Cálculo Privados ---

std::pair<double, double> GeometricFeatureCalculator::calculate_gc1_centroid(const CVRP& problem, FeatureSet& features) const {
    double sum_x = 0.0;
    double sum_y = 0.0;
    const auto& nodes = problem.getAllNodes();
    
    for (const auto& pair : nodes) {
        sum_x += pair.second->getX();
        sum_y += pair.second->getY();
    }
    
    const double N = problem.getDimension();
    const double centroid_x = sum_x / N;
    const double centroid_y = sum_y / N;

    features["GC1.1_centroid_x"] = centroid_x;
    features["GC1.2_centroid_y"] = centroid_y;
    
    return {centroid_x, centroid_y};
}

void GeometricFeatureCalculator::calculate_gc2_dist_to_centroid(const CVRP& problem, const std::pair<double, double>& centroid, FeatureSet& features) const {
    std::vector<double> distances;
    distances.reserve(problem.getDimension());

    for (const auto& pair : problem.getAllNodes()) {
        const double dx = pair.second->getX() - centroid.first;
        const double dy = pair.second->getY() - centroid.second;
        // std::hypot é uma forma segura de calcular sqrt(dx*dx + dy*dy)
        distances.push_back(std::hypot(dx, dy));
    }

    auto stats = StatisticsCalculator::calculateAll(distances);
    auto modal_stats = StatisticsCalculator::analyzeModes(distances);
    
    features["GC2.1_mean_dist_centroid"] = stats.mean;
    features["GC2.1_stddev_dist_centroid"] = stats.std_dev;
    features["GC2.1_min_dist_centroid"] = stats.min;
    features["GC2.1_max_dist_centroid"] = stats.max;
    features["GC2.1_median_dist_centroid"] = stats.median;
    features["GC2.1_skew_dist_centroid"] = stats.skewness;
    features["GC2.1_kurt_dist_centroid"] = stats.kurtosis;
    features["GC2.1_cv_dist_centroid"] = stats.cv;
    features["GC2.1_q1_dist_centroid"] = stats.q1;
    features["GC2.1_q3_dist_centroid"] = stats.q3;
    features["GC2.1_num_modes_dist_centroid"] = static_cast<double>(modal_stats.num_modes);
}

GeometricContext GeometricFeatureCalculator::calculate_gc3_bounding_box(const CVRP& problem, FeatureSet& features) const {
    GeometricContext context; 
    if (problem.getDimension() < 1) return {};
    double min_x = std::numeric_limits<double>::max(), max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max(), max_y = std::numeric_limits<double>::lowest();
    for (const auto& pair : problem.getAllNodes()) {
        min_x = std::min(min_x, pair.second->getX()); 
        max_x = std::max(max_x, pair.second->getX());
        min_y = std::min(min_y, pair.second->getY()); 
        max_y = std::max(max_y, pair.second->getY());
    }
    const double width = max_x - min_x;
    const double height = max_y - min_y;

    features["GC3.1_bounding_box_area"] = width * height;
    features["GC3.2_bbox_aspect_ratio"] = (std::max(width, height) > 0) ? (std::min(width, height) / std::max(width, height)) : 1.0;
    
    const double delta = 0.05 * std::min(width, height);
    int nodes_near_edge = 0;
    for (const auto& pair : problem.getAllNodes()) {
        const double x = pair.second->getX(); const double y = pair.second->getY();
        if (x <= min_x + delta || x >= max_x - delta || y <= min_y + delta || y >= max_y - delta) {
            nodes_near_edge++;
        }
    }
    features["GC3.3_frac_nodes_near_bbox_edge"] = static_cast<double>(nodes_near_edge) / problem.getDimension();

    context.bb_width = width;
    context.bb_height = height;

      return context;
}

/**
 * @brief Calcula as features do casco convexo (GC4) usando a Boost.Geometry.
 * 
 * Esta função determina o menor polígono convexo que envolve todos os pontos
 * e calcula sua área e a fração de nós que se encontram em sua borda.
 * 
 * @param problem A instância CVRP a ser analisada.
 * @param features O mapa onde as novas features (GC4.1, GC4.2) serão inseridas.
 */
polygon_2d GeometricFeatureCalculator::calculate_gc4_convex_hull(const CVRP& problem, FeatureSet& features) const {
    const auto& nodes = problem.getAllNodes();
    polygon_2d hull;
    if (nodes.size() < 3) {
        features["GC4.1_convex_hull_area"] = 0.0;
        features["GC4.2_fraction_nodes_on_hull"] = nodes.empty() ? 0.0 : 1.0;
        return hull;
    }

    multi_point_2d multi_points;
    for (const auto& pair : nodes) {
        multi_points.emplace_back(pair.second->getX(), pair.second->getY());
    }

    boost::geometry::convex_hull(multi_points, hull);
    
    features["GC4.1_convex_hull_area"] = boost::geometry::area(hull);
    const auto& hull_points = hull.outer();
    size_t nodes_on_hull_count = hull_points.empty() ? 0 : hull_points.size() - 1;
    features["GC4.2_fraction_nodes_on_hull"] = static_cast<double>(nodes_on_hull_count) / nodes.size();

    // GC4.3: Distância dos pontos internos ao casco
    std::vector<double> inner_point_distances;
    for (const auto& p : multi_points) {
        bool is_on_hull = false;
        for(const auto& hp : hull_points) {
            if (boost::geometry::equals(p, hp)) {
                is_on_hull = true;
                break;
            }
        }
        if (!is_on_hull) {
            inner_point_distances.push_back(boost::geometry::distance(p, hull));
        }
    }
    if (!inner_point_distances.empty()) {
        auto stats_inner = StatisticsCalculator::calculateAll(inner_point_distances);
        auto modal_stats_inner = StatisticsCalculator::analyzeModes(inner_point_distances);
        
        features["GC4.3_mean_dist_inner_to_hull"] = stats_inner.mean;
        features["GC4.3_stddev_dist_inner_to_hull"] = stats_inner.std_dev;
        features["GC4.3_min_dist_inner_to_hull"] = stats_inner.min;
        features["GC4.3_max_dist_inner_to_hull"] = stats_inner.max;
        features["GC4.3_median_dist_inner_to_hull"] = stats_inner.median;
        features["GC4.3_skewness_dist_inner_to_hull"] = stats_inner.skewness;
        features["GC4.3_kurtosis_dist_inner_to_hull"] = stats_inner.kurtosis;
        features["GC4.3_cv_dist_inner_to_hull"] = stats_inner.cv;
        features["GC4.3_q1_dist_inner_to_hull"] = stats_inner.q1;
        features["GC4.3_q3_dist_inner_to_hull"] = stats_inner.q3;
        features["GC4.3_num_modes_dist_inner_to_hull"] = static_cast<double>(modal_stats_inner.num_modes);
    }
    
    // GC4.4: Comprimento das arestas do casco
    std::vector<double> hull_edge_lengths;
    if (nodes_on_hull_count > 1) {
        for (size_t i = 0; i < nodes_on_hull_count; ++i) {
            hull_edge_lengths.push_back(boost::geometry::distance(hull_points[i], hull_points[i+1]));
        }
    }
    if (!hull_edge_lengths.empty()) {
        auto stats_edges = StatisticsCalculator::calculateAll(hull_edge_lengths);
        auto modal_stats_edges = StatisticsCalculator::analyzeModes(hull_edge_lengths);

        features["GC4.4_mean_hull_edge_length"] = stats_edges.mean;
        features["GC4.4_stddev_hull_edge_length"] = stats_edges.std_dev;
        features["GC4.4_min_hull_edge_length"] = stats_edges.min;
        features["GC4.4_max_hull_edge_length"] = stats_edges.max;
        features["GC4.4_median_hull_edge_length"] = stats_edges.median;
        features["GC4.4_skewness_hull_edge_length"] = stats_edges.skewness;
        features["GC4.4_kurtosis_hull_edge_length"] = stats_edges.kurtosis;
        features["GC4.4_cv_hull_edge_length"] = stats_edges.cv;
        features["GC4.4_q1_hull_edge_length"] = stats_edges.q1;
        features["GC4.4_q3_hull_edge_length"] = stats_edges.q3;
        features["GC4.4_num_modes_hull_edge_length"] = static_cast<double>(modal_stats_edges.num_modes);
    }
    return hull;
}

void GeometricFeatureCalculator::calculate_gc5_AngleFeatures(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 3) return;
    std::vector<double> angles;
    const auto& nodes_map = problem.getAllNodes();
    std::vector<const Node*> node_vec;
    node_vec.reserve(nodes_map.size());
    for(const auto& pair : nodes_map) node_vec.push_back(pair.second.get());

    for (const auto* node_k : node_vec) {
        double min_dist1 = std::numeric_limits<double>::max(), min_dist2 = std::numeric_limits<double>::max();
        const Node* neighbor_i = nullptr, *neighbor_j = nullptr;
        for (const auto* other_node : node_vec) {
            if (node_k == other_node) continue;
            double dist = std::hypot(node_k->getX() - other_node->getX(), node_k->getY() - other_node->getY());
            if (dist < min_dist1) {
                min_dist2 = min_dist1; 
                neighbor_j = neighbor_i;
                min_dist1 = dist; 
                neighbor_i = other_node;
            } else if (dist < min_dist2) {
                min_dist2 = dist; 
                neighbor_j = other_node;
            }
        }
        if (neighbor_i && neighbor_j) {
            const double v_ki_x = neighbor_i->getX() - node_k->getX(), v_ki_y = neighbor_i->getY() - node_k->getY();
            const double v_kj_x = neighbor_j->getX() - node_k->getX(), v_kj_y = neighbor_j->getY() - node_k->getY();
            const double dot_product = v_ki_x * v_kj_x + v_ki_y * v_kj_y;
            const double mag_ki = std::hypot(v_ki_x, v_ki_y), mag_kj = std::hypot(v_kj_x, v_kj_y);
            if (mag_ki > 0 && mag_kj > 0) {
                double cos_val = dot_product / (mag_ki * mag_kj);
                angles.push_back(std::acos(std::clamp(cos_val, -1.0, 1.0)));
            }
        }
    }
    if (angles.empty()) return;
    auto stats = StatisticsCalculator::calculateAll(angles);
    features["GC5.1_mean_angles"] = stats.mean;
    features["GC5.1_stddev_angles"] = stats.std_dev;
    features["GC5.1_min_angle"] = stats.min;
    features["GC5.1_max_angle"] = stats.max;
    features["GC5.1_median_angle"] = stats.median;
}

void GeometricFeatureCalculator::calculate_gc6_CosineAngleFeatures(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 3) return;
    std::vector<double> cosines;
    const auto& nodes_map = problem.getAllNodes();
    std::vector<const Node*> node_vec;
    node_vec.reserve(nodes_map.size());
    for(const auto& pair : nodes_map) node_vec.push_back(pair.second.get());

    for (const auto* node_k : node_vec) {
        double min_dist1 = std::numeric_limits<double>::max(), min_dist2 = std::numeric_limits<double>::max();
        const Node* neighbor_i = nullptr, *neighbor_j = nullptr;
        for (const auto* other_node : node_vec) {
            if (node_k == other_node) continue;
            double dist = std::hypot(node_k->getX() - other_node->getX(), node_k->getY() - other_node->getY());
            if (dist < min_dist1) {
                min_dist2 = min_dist1; neighbor_j = neighbor_i;
                min_dist1 = dist; neighbor_i = other_node;
            } else if (dist < min_dist2) {
                min_dist2 = dist; neighbor_j = other_node;
            }
        }
        if (neighbor_i && neighbor_j) {
            const double v_ki_x = neighbor_i->getX() - node_k->getX(), v_ki_y = neighbor_i->getY() - node_k->getY();
            const double v_kj_x = neighbor_j->getX() - node_k->getX(), v_kj_y = neighbor_j->getY() - node_k->getY();
            const double dot_product = v_ki_x * v_kj_x + v_ki_y * v_kj_y;
            const double mag_ki = std::hypot(v_ki_x, v_ki_y), mag_kj = std::hypot(v_kj_x, v_kj_y);
            if (mag_ki > 0 && mag_kj > 0) {
                cosines.push_back(dot_product / (mag_ki * mag_kj));
            }
        }
    }
    if (cosines.empty()) return;

    auto stats = StatisticsCalculator::calculateAll(cosines);
    auto modal_stats = StatisticsCalculator::analyzeModes(cosines, 0.1);
    features["GC6.1_mean_cosine_angles"] = stats.mean;
    features["GC6.1_stddev_cosine_angles"] = stats.std_dev;
    features["GC6.1_min_cosine_angles"] = stats.min;
    features["GC6.1_max_cosine_angles"] = stats.max;
    features["GC6.1_median_cosine_angles"] = stats.median;
    features["GC6.1_skewness_cosine_angles"] = stats.skewness;
    features["GC6.1_kurtosis_cosine_angles"] = stats.kurtosis;
    features["GC6.1_cv_cosine_angles"] = stats.cv;
    features["GC6.1_q1_cosine_angles"] = stats.q1;
    features["GC6.1_q3_cosine_angles"] = stats.q3;
    features["GC6.1_num_modes_cosine_angles"] = static_cast<double>(modal_stats.num_modes);
}

void GeometricFeatureCalculator::calculate_gc7_voronoi(const CVRP& problem, const polygon_2d& hull, FeatureSet& features) const {
    const auto& nodes = problem.getAllNodes();
    if (nodes.size() < 3) return;

    std::vector<Node> node_objects;
    node_objects.reserve(nodes.size());
    for(const auto& pair : nodes) node_objects.push_back(*(pair.second.get()));
    
    boost::polygon::voronoi_diagram<double> diagram;
    build_voronoi_diagram(node_objects, &diagram);

    std::vector<double> cell_areas;
    cell_areas.reserve(diagram.num_cells());
    const double scale_factor = 1000.0; // O mesmo usado nos traits

    for (const auto& cell : diagram.cells()) {
        if (!cell.contains_point()) continue;
        const auto* edge = cell.incident_edge();
        if (edge == nullptr) continue;
        
        std::vector<point_2d> cell_vertices;
        bool is_infinite = false;
        const auto* start_edge = edge;
        do {
            if (edge->is_infinite()) {
                is_infinite = true;
                break;
            }
            cell_vertices.emplace_back(edge->vertex0()->x() / scale_factor, edge->vertex0()->y() / scale_factor);
            edge = edge->next();
        } while (edge != start_edge);
        
        if (!is_infinite) {
            cell_areas.push_back(shoelace_formula(cell_vertices));
        }
    }
    
    if (cell_areas.empty()) return;
    
    // Calculamos as estatísticas diretamente sobre as áreas brutas.
    auto stats = StatisticsCalculator::calculateAll(cell_areas);

    // As chaves agora refletem que são valores brutos.
    features["GC7.1_min_voronoi_area"] = stats.min;
    features["GC7.1_max_voronoi_area"] = stats.max;
    features["GC7.1_mean_voronoi_area"] = stats.mean;
    features["GC7.1_stddev_voronoi_area"] = stats.std_dev;
    features["GC7.1_skew_voronoi_area"] = stats.skewness;
    features["GC7.1_kurt_voronoi_area"] = stats.kurtosis;
}

void GeometricFeatureCalculator::calculate_gc8_delaunay(const CVRP& problem, const polygon_2d& hull, FeatureSet& features) const {
    const auto& nodes = problem.getAllNodes();
    if (nodes.size() < 3) return;

    std::vector<Node> node_objects;
    std::vector<const Node*> node_pointers;
    node_objects.reserve(nodes.size());
    node_pointers.reserve(nodes.size());
    for(const auto& pair : nodes) {
        node_objects.push_back(*(pair.second.get()));
        node_pointers.push_back(pair.second.get());
    }
    boost::polygon::voronoi_diagram<double> diagram;
    build_voronoi_diagram(node_objects, &diagram);
   
    std::vector<double> triangle_areas;
    for (const auto& vertex : diagram.vertices()) {
        if (vertex.incident_edge() == nullptr) continue;

        std::vector<const Node*> triangle_nodes;
        const auto* edge = vertex.incident_edge();
        const auto* start_edge = edge; 
        do {
            if (edge->is_primary()) {
                triangle_nodes.push_back(node_pointers[edge->cell()->source_index()]);
            }
            edge = edge->rot_next();
        } while (edge != start_edge);
        
        std::sort(triangle_nodes.begin(), triangle_nodes.end());
        triangle_nodes.erase(std::unique(triangle_nodes.begin(), triangle_nodes.end()), triangle_nodes.end());

        if (triangle_nodes.size() == 3) {
            triangle_areas.push_back(
                triangle_area(triangle_nodes[0], triangle_nodes[1], triangle_nodes[2])
            );
        }
    }

    if (triangle_areas.empty()) return;

    // Calculamos as estatísticas diretamente sobre as áreas brutas dos triângulos.
    auto stats = StatisticsCalculator::calculateAll(triangle_areas);

    // As chaves agora refletem que são valores brutos.
    features["GC8.1_min_delaunay_area"] = stats.min;
    features["GC8.1_max_delaunay_area"] = stats.max;
    features["GC8.1_mean_delaunay_area"] = stats.mean;
    features["GC8.1_stddev_delaunay_area"] = stats.std_dev;
    features["GC8.1_skew_delaunay_area"] = stats.skewness;
    features["GC8.1_kurt_delaunay_area"] = stats.kurtosis;
}