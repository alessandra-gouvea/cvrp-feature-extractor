
#include <cvrp-feature-extractor/extractors/GeometricFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>
#include <cmath>
#include <vector>
#include <limits>

// Função de alto nível que chama os métodos privados em ordem
void GeometricFeatureCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() == 0) return;

    // GC1 e GC2 estão ligados, pois GC2 precisa do resultado de GC1
    const auto centroid = calculate_gc1_centroid(problem, features);
    calculate_gc2_dist_to_centroid(problem, centroid, features);
    
    // GC3
    calculate_gc3_bounding_box(problem, features);
    
    // GC5
    calculate_gc5_AngleFeatures(problem, features);
}

// --- Implementação dos Métodos Privados ---

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

    features["GC2.1_mean_dist_centroid"] = stats.mean;
    features["GC2.2_stddev_dist_centroid"] = stats.std_dev;
    features["GC2.3_skew_dist_centroid"] = stats.skewness;
    features["GC2.4_kurt_dist_centroid"] = stats.kurtosis;
    features["GC2.5_cv_dist_centroid"] = stats.cv;
    features["GC2.6_min_dist_centroid"] = stats.min;
    features["GC2.7_max_dist_centroid"] = stats.max;
    features["GC2.8_median_dist_centroid"] = stats.median;
}

void GeometricFeatureCalculator::calculate_gc3_bounding_box(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 2) return;

    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();

    for (const auto& pair : problem.getAllNodes()) {
        min_x = std::min(min_x, pair.second->getX());
        max_x = std::max(max_x, pair.second->getX());
        min_y = std::min(min_y, pair.second->getY());
        max_y = std::max(max_y, pair.second->getY());
    }

    const double width = max_x - min_x;
    const double height = max_y - min_y;

    features["GC3.1_bounding_box_area"] = width * height;

    // Para GC3.2 e GC3.3, usaremos um delta de 5% da menor dimensão da caixa
    const double delta = 0.05 * std::min(width, height);
    int nodes_near_edge = 0;
    for (const auto& pair : problem.getAllNodes()) {
        const double x = pair.second->getX();
        const double y = pair.second->getY();
        if (x <= min_x + delta || x >= max_x - delta ||
            y <= min_y + delta || y >= max_y - delta) {
            nodes_near_edge++;
        }
    }

    features["GC3.2_nodes_near_bbox_edge"] = static_cast<double>(nodes_near_edge);
    features["GC3.3_frac_nodes_near_bbox_edge"] = static_cast<double>(nodes_near_edge) / problem.getDimension();
}

void GeometricFeatureCalculator::calculate_gc5_AngleFeatures(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 3) return; // Ângulos requerem 3 pontos

    std::vector<double> angles;
    angles.reserve(problem.getDimension());
    const auto& nodes = problem.getAllNodes();

    // Converte o mapa para um vetor para acesso rápido por índice, se necessário
    std::vector<const Node*> node_vec;
    node_vec.reserve(nodes.size());
    for(const auto& pair : nodes) node_vec.push_back(pair.second.get());

    for (const auto* node_k : node_vec) {
        double min_dist1 = std::numeric_limits<double>::max();
        double min_dist2 = std::numeric_limits<double>::max();
        const Node* neighbor_i = nullptr;
        const Node* neighbor_j = nullptr;

        // Encontra os 2 vizinhos mais próximos para node_k
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
        
        // Calcula o ângulo entre os vetores (k->i) e (k->j)
        if (neighbor_i && neighbor_j) {
            const double v_ki_x = neighbor_i->getX() - node_k->getX();
            const double v_ki_y = neighbor_i->getY() - node_k->getY();
            const double v_kj_x = neighbor_j->getX() - node_k->getX();
            const double v_kj_y = neighbor_j->getY() - node_k->getY();

            const double dot_product = (v_ki_x * v_kj_x) + (v_ki_y * v_kj_y);
            const double mag_ki = std::hypot(v_ki_x, v_ki_y);
            const double mag_kj = std::hypot(v_kj_x, v_kj_y);

            if (mag_ki > 0 && mag_kj > 0) {
                double angle = std::acos(dot_product / (mag_ki * mag_kj));
                angles.push_back(angle); // Ângulo em radianos
            }
        }
    }
    
    if (angles.empty()) return;
    
    auto stats = StatisticsCalculator::calculateAll(angles);

    features["GC5.1_mean_angles"] = stats.mean;
    features["GC5.2_stddev_angles"] = stats.std_dev;
    features["GC5.3_min_angle"] = stats.min;
    features["GC5.4_max_angle"] = stats.max;
    features["GC5.5_median_angle"] = stats.median;
}