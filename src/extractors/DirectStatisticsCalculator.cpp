// ===================================================================
// == CÁLCULO DAS FEATURES DE ESTATÍSTICA DIRETA (DS1, DS2, DS3, DS4) 
// ===================================================================

#include <cvrp-feature-extractor/extractors/DirectStatisticsCalculator.h>
#include <numeric>
#include <cmath> 
#include <set>
#include <iostream>
#include <vector>

namespace {
    double round_to_precision(double value, int precision) {
        const double multiplier = std::pow(10.0, precision);
        return std::round(value * multiplier) / multiplier;
    }
}

static double calculateDistance(const Node* a, const Node* b) {
    if (!a || !b) return 0.0;
    double dx = a->getX() - b->getX();
    double dy = a->getY() - b->getY();
    return std::sqrt(dx * dx + dy * dy);
}

// Este é o único método público.
void DirectStatisticsCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 2) {
        return;
    }
    std::vector<double> all_distances = calculate_ds_features(problem, features);
}

// --- IMPLEMENTAÇÃO DOS ESPECIALISTAS PRIVADOS ---

/**
 * @brief DS1: Calcula as estatísticas sobre a distribuição dos valores da matriz de distância.
 */
std::vector<double> DirectStatisticsCalculator::calculate_ds_features(const CVRP& problem, FeatureSet& features) const {
    const size_t N = problem.getDimension();
    const auto& nodes_map = problem.getAllNodes();

    std::vector<const Node*> nodes_vec;
    nodes_vec.reserve(N);
    for(const auto& pair : nodes_map) {
        nodes_vec.push_back(pair.second.get());
    }

    // Constrói o multiconjunto S_D
    std::vector<double> all_distances;
    all_distances.reserve(N * (N - 1) / 2);
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i + 1; j < N; ++j) {
            all_distances.push_back(calculateDistance(nodes_vec[i], nodes_vec[j]));
        }
    }
    
     // DS1.1: 11-Descriptor Bundle
    auto sorted_distances = all_distances;
    auto stats = StatisticsCalculator::calculateAll(sorted_distances);
    auto modal_stats = StatisticsCalculator::analyzeModes(all_distances);

    features["DS1.1_mean_distances"] = stats.mean;
    features["DS1.1_stddev_distances"] = stats.std_dev;
    features["DS1.1_min_distance"] = stats.min;
    features["DS1.1_max_distance"] = stats.max;
    features["DS1.1_median_distance"] = stats.median;
    features["DS1.1_skewness_distances"] = stats.skewness;
    features["DS1.1_kurtosis_distances"] = stats.kurtosis;
    features["DS1.1_cv_distances"] = stats.cv;
    features["DS1.1_q1_distances"] = stats.q1;
    features["DS1.1_q3_distances"] = stats.q3;
    features["DS1.1_num_modes_distances"] = static_cast<double>(modal_stats.num_modes);

     // DS1.2: SLEV
    features["DS1.2_SLEV"] = (sorted_distances.size() >= N) ? 
        std::accumulate(sorted_distances.begin(), sorted_distances.begin() + N, 0.0) : 0.0;

    // DS1.3: SL25P
    size_t k25 = sorted_distances.size() / 4;
    features["DS1.3_SL25P"] = std::accumulate(sorted_distances.begin(), sorted_distances.begin() + k25, 0.0);

    // DS1.4: SH25P
    features["DS1.4_SH25P"] = std::accumulate(sorted_distances.end() - k25, sorted_distances.end(), 0.0);

    long count_below_mean = 0;
    long count_above_mean = 0;
    long count_below_median = 0;
    for (const auto& dist : all_distances) {
        if (dist < stats.mean) count_below_mean++;
        if (dist > stats.mean) count_above_mean++;
        if (dist < stats.median) count_below_median++;
    }
    features["DS1.5_CEB_mean"] = static_cast<double>(count_below_mean);
    features["DS1.6_CEA_mean"] = static_cast<double>(count_above_mean);
    features["DS1.7_CEB_median"] = static_cast<double>(count_below_median);

    features["DS1.8_ERTL"] = N * stats.mean;

    // --- DS2: Modal Analysis of the Distance Distribution ---
    features["DS2.1_primary_mode_value"] = modal_stats.primary_mode_value;
    features["DS2.2_primary_mode_freq_norm"] = modal_stats.primary_mode_freq_norm;
    features["DS2.3_mean_modal_values"] = modal_stats.mean_of_modal_values;

    // --- DS3: Fraction of Distinct Distances at Various Precisions ---
    for (int p : {1, 2, 3, 4}) {
        std::set<double> distinct_rounded_distances;
        for (double dist : all_distances) {
            distinct_rounded_distances.insert(round_to_precision(dist, p));
        }
        std::string key = "DS3." + std::to_string(p) + "_frac_distinct_prec" + std::to_string(p);
        features[key] = static_cast<double>(distinct_rounded_distances.size()) / all_distances.size();
    }

    // --- DS4: Distance Matrix Symmetry Classification ---
    // --- DS5: Triangle Inequality Adherence --- O(N^3)

    return all_distances; // Retorna o vetor para reutilização por outras funções.
}