#include <cvrp-feature-extractor/extractors/VRPSpecificFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>

#include <vector>
#include <numeric>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void VRPSpecificFeatureCalculator::calculate(const CVRP& problem, const ClusteringResult& clustering_result, FeatureSet& features) const {
    const int N = problem.getDimension();
    const double Q = problem.getCapacity();
    const Node* depot = problem.getDepot();

    if (N < 2 || Q <= 0 || depot == nullptr) {
        return; // As features VS não são significativas sem múltiplos nós, capacidade e depósito.
    }

    // --- Coleta de Dados Iniciais ---
    std::vector<const Node*> customers;
    std::vector<double> demands;
    double total_demand = 0.0;

    for (const auto& pair : problem.getAllNodes()) {
        const auto& node = pair.second;
        if (!node->isDepot()) {
            customers.push_back(node.get());
            demands.push_back(node->getDemand());
            total_demand += node->getDemand();
        }
    }
    const int num_customers = customers.size();
    if (num_customers == 0) return;

    // --- VS1: Fundamental VRP Parameters ---
    features["VS1.1_num_customers"] = static_cast<double>(num_customers);
    features["VS1.2_vehicle_capacity"] = Q;

    // --- VS2: Depot-Related Structural Features ---
    features["VS2.1_depot_x"] = depot->getX();
    features["VS2.1_depot_y"] = depot->getY();

    // Calcula o centroide apenas dos clientes
    double cust_sum_x = 0.0, cust_sum_y = 0.0;
    for (const auto* node : customers) {
        cust_sum_x += node->getX();
        cust_sum_y += node->getY();
    }
    const double cust_centroid_x = cust_sum_x / num_customers;
    const double cust_centroid_y = cust_sum_y / num_customers;
    features["VS2.2_depot_eccentricity"] = std::hypot(depot->getX() - cust_centroid_x, depot->getY() - cust_centroid_y);

    std::vector<double> depot_distances;
    depot_distances.reserve(num_customers);
    for (const auto* node : customers) {
        depot_distances.push_back(std::hypot(node->getX() - depot->getX(), node->getY() - depot->getY()));
    }
    auto stats_depot_dist = StatisticsCalculator::calculateAll(depot_distances);
    features["VS2.3_mean_dist_cust_depot"] = stats_depot_dist.mean;
    features["VS2.3_stddev_dist_cust_depot"] = stats_depot_dist.std_dev;
    features["VS2.3_min_dist_cust_depot"] = stats_depot_dist.min;
    features["VS2.3_max_dist_cust_depot"] = stats_depot_dist.max;
    features["VS2.3_median_dist_cust_depot"] = stats_depot_dist.median;

    // --- VS3: Depot Position Category ---
    double min_cx = std::numeric_limits<double>::max(), max_cx = std::numeric_limits<double>::lowest();
    double min_cy = std::numeric_limits<double>::max(), max_cy = std::numeric_limits<double>::lowest();
    for (const auto* node : customers) {
        min_cx = std::min(min_cx, node->getX()); max_cx = std::max(max_cx, node->getX());
        min_cy = std::min(min_cy, node->getY()); max_cy = std::max(max_cy, node->getY());
    }
    bool is_outside = (depot->getX() < min_cx || depot->getX() > max_cx || 
                       depot->getY() < min_cy || depot->getY() > max_cy);
    features["VS3.1_IsDepotOutside"] = is_outside ? 1.0 : 0.0;

    // --- VS4: Customer Polar Angles ---
    std::vector<double> polar_angles;
    polar_angles.reserve(num_customers);
    for (const auto* node : customers) {
        polar_angles.push_back(std::atan2(node->getY() - depot->getY(), node->getX() - depot->getX()));
    }
    // O cálculo de estatísticas circulares é complexo. Usamos o desvio padrão linear como uma aproximação.
    // Para um cálculo correto, seria necessário converter ângulos para vetores unitários e analisar.
    auto stats_angles = StatisticsCalculator::calculateAll(polar_angles);
    features["VS4.1_circ_stddev_angles"] = stats_angles.std_dev; // Aproximação
    // A largura de banda (bandwidth) é um hiperparâmetro. 0.1 rad (~6 graus) é um começo razoável.
    features["VS4.2_num_modes_angles"] = static_cast<double>(
        StatisticsCalculator::analyzeCircularModes(polar_angles, 360, 0.1)
    );

    // --- VS5: Customer Demand Features ---
    auto stats_demands = StatisticsCalculator::calculateAll(demands);
    features["VS5.1_mean_demand"] = stats_demands.mean;
    features["VS5.1_stddev_demand"] = stats_demands.std_dev;
    features["VS5.1_min_demand"] = stats_demands.min;
    features["VS5.1_max_demand"] = stats_demands.max;
    features["VS5.1_median_demand"] = stats_demands.median;
    features["VS5.1_cv_demand"] = stats_demands.cv;

    // --- VS6: Capacity Constraint Tightness Features ---
    double k_min = std::ceil(total_demand / Q);
    features["VS6.1_min_theoretical_fleet_size"] = k_min;
    if (k_min > 0) {
        features["VS6.2_capacity_tightness_ratio"] = total_demand / (k_min * Q);
    }
    features["VS6.3_individual_customer_pressure"] = stats_demands.max / Q;
    features["VS6.4_avg_cust_per_route_est"] = static_cast<double>(num_customers) / k_min;
    
    // --- VS7: Hybrid Demand and Constraint Features ---
     // Para mapear os índices (0 a N-1) do resultado do clustering de volta para os nós
    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    nodes_by_index.push_back(depot); // Garante que o índice 0 é o depósito
    for (const auto* customer : customers) {
        nodes_by_index.push_back(customer);
    }

    // VS7.1: Max Cluster Demand Ratio
    double max_cluster_demand = 0.0;
    if (!clustering_result.points_in_cluster.empty()) {
        for (const auto& pair : clustering_result.points_in_cluster) {
            double current_cluster_demand = 0.0;
            for (size_t point_idx : pair.second) {
                // Acessa o nó usando o índice e obtém sua demanda
                current_cluster_demand += nodes_by_index[point_idx]->getDemand();
            }
            if (current_cluster_demand > max_cluster_demand) {
                max_cluster_demand = current_cluster_demand;
            }
        }
    }
    features["VS7.1_max_cluster_demand_ratio"] = max_cluster_demand / Q;
    
    // VS7.2: Outlier Demand Ratio
    double outlier_demand = 0.0;
    if (!clustering_result.outlier_indices.empty()) {
        for (size_t point_idx : clustering_result.outlier_indices) {
            outlier_demand += nodes_by_index[point_idx]->getDemand();
        }
    }
    if (total_demand > 1e-9) {
        features["VS7.2_outlier_demand_ratio"] = outlier_demand / total_demand;
    } else {
        features["VS7.2_outlier_demand_ratio"] = 0.0;
    }
    
    // VS7.3: Normalized Demand Variability (já estava correta)
    features["VS7.3_norm_demand_variability"] = stats_demands.std_dev / Q;
}

