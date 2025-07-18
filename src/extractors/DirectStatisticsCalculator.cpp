// ===================================================================
// == CÁLCULO DAS FEATURES DE ESTATÍSTICA DIRETA (DS1, DS2, DS3, DS4) 
// ===================================================================

#include <cvrp-feature-extractor/extractors/DirectStatisticsCalculator.h>
#include <numeric>
#include <cmath> 
#include <set>
#include <iostream>

static double calculateDistance(const Node* a, const Node* b) {
    if (!a || !b) return 0.0;
    double dx = a->getX() - b->getX();
    double dy = a->getY() - b->getY();
    return std::sqrt(dx * dx + dy * dy);
}

// Este é o único método público. Ele agora é curto, limpo e fácil de ler.
void DirectStatisticsCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    if (problem.getDimension() < 2) {
        return;
    }
    // A função DS1 calcula e retorna as distâncias para reutilização.
    std::vector<double> all_distances = calculate_ds1_features(problem, features);
    // As outras funções usam os dados pré-calculados quando possível.
    calculate_ds2_features(all_distances, features);
    calculate_ds3_features(problem, features);
    calculate_ds4_features(problem, features);
}

// --- IMPLEMENTAÇÃO DOS ESPECIALISTAS PRIVADOS ---

/**
 * @brief DS1: Calcula as estatísticas sobre a distribuição dos valores da matriz de distância.
 */
std::vector<double> DirectStatisticsCalculator::calculate_ds1_features(const CVRP& problem, FeatureSet& features) const {
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
    
    // DS1.1 a DS1.8 e outras estatísticas gerais
    auto sorted_distances = all_distances; 
    auto stats = StatisticsCalculator::calculateAll(sorted_distances);

    features["DS1.1_mean_distances"] = stats.mean;
    features["DS1.2_stddev_distances"] = stats.std_dev;
    features["DS1.3_skewness_distances"] = stats.skewness;
    features["DS1.4_kurtosis_distances"] = stats.kurtosis;
    features["DS1.5_cv_distances"] = stats.cv;
    features["DS1.6_min_distance"] = stats.min;
    features["DS1.7_max_distance"] = stats.max;
    features["DS1.8_median_distance"] = stats.median;
    
    // DS1.9 a DS1.11: Estatísticas de Moda
    std::map<double, int> counts;
    for (const double dist : all_distances) {
        counts[dist]++;
    }
    int max_freq = 0;
    for (const auto& pair : counts) {
        if (pair.second > max_freq) {
            max_freq = pair.second;
        }
    }
    std::vector<double> modes;
    for (const auto& pair : counts) {
        if (pair.second == max_freq) {
            modes.push_back(pair.first);
        }
    }
    features["DS1.9_num_distinct_modes"] = static_cast<double>(modes.size());
    features["DS1.10_freq_of_mode"] = static_cast<double>(max_freq);
    features["DS1.11_mode_of_distances"] = std::accumulate(modes.begin(), modes.end(), 0.0) / modes.size();

    // DS1.12 a DS1.18: Contagens e Somas baseadas em Ordenação/Média
    if (sorted_distances.size() >= N) {
        features["DS1.12_SLEV"] = std::accumulate(sorted_distances.begin(), sorted_distances.begin() + N, 0.0);
    }
    long count_below_mean = 0, count_above_mean = 0, count_below_median = 0;
    for(const auto& dist : all_distances) {
        if (dist < stats.mean) count_below_mean++;
        if (dist > stats.mean) count_above_mean++;
        if (dist < stats.median) count_below_median++;
    }
    features["DS1.13_count_edges_below_mean"] = static_cast<double>(count_below_mean);
    features["DS1.14_prop_edges_below_mean"] = static_cast<double>(count_below_mean) / all_distances.size();
    features["DS1.15_count_edges_below_median"] = static_cast<double>(count_below_median);
    features["DS1.17_count_edges_above_mean"] = static_cast<double>(count_above_mean);

    size_t k25 = static_cast<size_t>(std::floor(sorted_distances.size() * 0.25));
    if (k25 > 0) {
        features["DS1.16_sum_lowest_25p"] = std::accumulate(sorted_distances.begin(), sorted_distances.begin() + k25, 0.0);
        features["DS1.18_sum_highest_25p"] = std::accumulate(sorted_distances.end() - k25, sorted_distances.end(), 0.0);
    }
    
    // DS1.19: Estimated Random Tour Length (ERTL)
    features["DS1.19_ERTL"] = N * stats.mean;

    return all_distances; // Retorna o vetor para reutilização por outras funções.
}

/**
 * @brief DS2: Calcula a fração de distâncias distintas em várias precisões.
 */
void DirectStatisticsCalculator::calculate_ds2_features(const std::vector<double>& all_distances, FeatureSet& features) const {
    for (int p = 1; p <= 4; ++p) {
        double multiplier = std::pow(10, p);
        std::set<long long> unique_rounded_distances;
        for (const double dist : all_distances) {
            unique_rounded_distances.insert(static_cast<long long>(std::round(dist * multiplier)));
        }
        std::string key = "DS2." + std::to_string(p) + "_frac_distinct_distances_" + std::to_string(p) + "dec";
        features[key] = static_cast<double>(unique_rounded_distances.size()) / all_distances.size();
    }
}

/**
 * @brief DS3: Classifica a simetria da matriz de distância.
 */
void DirectStatisticsCalculator::calculate_ds3_features(const CVRP& problem, FeatureSet& features) const {
    // Se o problema pudesse carregar uma matriz explícita, esta lógica precisara ser expandida.
    features["DS3.1_is_asymmetric"] = 0.0;
}

/**
 * @brief DS4: Verifica a aderência à desigualdade triangular.
 */
void DirectStatisticsCalculator::calculate_ds4_features(const CVRP& problem, FeatureSet& features) const {
    const size_t N = problem.getDimension();
    if (N < 3) {
        features["DS4.1_frac_non_violating_triplets"] = 1.0;
        return;
    }
    
    const auto& nodes_map = problem.getAllNodes();
    std::vector<const Node*> nodes_vec;
    nodes_vec.reserve(N);
    for(const auto& pair : nodes_map) {
        nodes_vec.push_back(pair.second.get());
    }

    // CUIDADO: Este algoritmo é de complexidade O(N³).
    // Pode ser muito lento para instâncias grandes (> ~500 nós).
    long long total_triplets = (N * (N - 1) * (N - 2)) / 6;
    long long violating_triplets = 0;
    const double tolerance = 1e-9;

    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i + 1; j < N; ++j) {
            double d_ij = calculateDistance(nodes_vec[i], nodes_vec[j]);
            for (size_t k = j + 1; k < N; ++k) {
                double d_ik = calculateDistance(nodes_vec[i], nodes_vec[k]);
                double d_jk = calculateDistance(nodes_vec[j], nodes_vec[k]);
                if ((d_ij > d_ik + d_jk + tolerance) ||
                    (d_ik > d_ij + d_jk + tolerance) ||
                    (d_jk > d_ij + d_ik + tolerance)) {
                    violating_triplets++;
                }
            }
        }
    }
    
    if (total_triplets > 0) {
        features["DS4.1_frac_non_violating_triplets"] = 1.0 - (static_cast<double>(violating_triplets) / total_triplets);
    } else {
        features["DS4.1_frac_non_violating_triplets"] = 1.0;
    }
    
    // A magnitude da violação (DS4.2) é deixada como placeholder.
    features["DS4.2_mean_norm_magnitude_violations"] = 0.0;
}