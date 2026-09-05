#include <cvrp-feature-extractor/extractors/ClusteringFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>

// --- Includes para mlpack e Armadillo ---
#include <mlpack/methods/dbscan/dbscan.hpp>
#include <armadillo> // mlpack usa as estruturas de dados da Armadillo

#include <vector>
#include <map>
#include <limits>

ClusteringResult ClusteringFeatureCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    if (N < 4)  return {}; // Retorna um ClusteringResult vazio

    // --- Passo 1: Preparar os dados para mlpack ---
    // A matriz de dados da mlpack deve ter Dimensoes x NumeroDePontos.
    arma::mat data(2, N); // 2 dimensões (x, y)
    int col_idx = 0;
    for (const auto& pair : problem.getAllNodes()) {
        data(0, col_idx) = pair.second->getX();
        data(1, col_idx) = pair.second->getY();
        col_idx++;
    }

    // --- Passo 2: Delegar o cálculo dos epsilons ---
    const auto epsilons_to_run = calculate_epsilons(data);
    if (epsilons_to_run.empty()) {
         return {}; // Retorna um ClusteringResult vazio
    }
    const size_t minPoints = 4; // Parâmetro padrão do DBSCAN - Smith-Miles et al. (2010)

    ClusteringResult elbow_result; 
    // --- Passo 3: Execução do DBSCAN para cada epsilon ---
    for (const auto& pair : epsilons_to_run) {
        const std::string& eps_suffix = pair.first;
        const double epsilon = pair.second;
        ClusteringResult current_result = run_dbscan_and_extract(data, epsilon, minPoints, eps_suffix, features);
        // Se esta foi a execução do "cotovelo", guardamos seu resultado.
        if (eps_suffix == "_epsElbow") {
            elbow_result = current_result;
        }
    }
    return elbow_result;
}

std::map<std::string, double> ClusteringFeatureCalculator::calculate_epsilons(const arma::mat& data) const {
    const int N = data.n_cols;
    const size_t minPoints = 4;
    
    std::map<std::string, double> epsilons_to_run;

    // --- a) Abordagem da Mediana 1-NN (para múltiplos fixos) ---
    std::vector<double> nn1_distances;
    nn1_distances.reserve(N);
    for (int i = 0; i < N; ++i) {
        double min_dist = std::numeric_limits<double>::max();
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
            // arma::norm calcula a distância euclidiana entre duas colunas de uma matriz
            double dist = arma::norm(data.col(i) - data.col(j));
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        nn1_distances.push_back(min_dist);
    }
    std::sort(nn1_distances.begin(), nn1_distances.end());
    double median_1nn_dist = nn1_distances[N / 2];

    if (median_1nn_dist > 1e-9) {
        epsilons_to_run["_epsX1.5"] = 1.5 * median_1nn_dist;
        epsilons_to_run["_epsX2.0"] = 2.0 * median_1nn_dist;
        epsilons_to_run["_epsX3.0"] = 3.0 * median_1nn_dist;
    }

    // --- b) Abordagem da Regra do Cotovelo ---
    const int k = minPoints - 1; // k = 3 para minPoints = 4
    std::vector<double> k_dist;
    k_dist.reserve(N);

    for(int i = 0; i < N; ++i) {
        std::vector<double> dists_to_others;
        dists_to_others.reserve(N - 1);
        for(int j = 0; j < N; ++j) {
            if (i == j) continue;
            dists_to_others.push_back(arma::norm(data.col(i) - data.col(j)));
        }
        std::sort(dists_to_others.begin(), dists_to_others.end());
        k_dist.push_back(dists_to_others[k-1]);
    }
    
    std::sort(k_dist.begin(), k_dist.end());
    
    // Encontra o ponto de maior curvatura ("cotovelo") numericamente
    double max_dist_from_line = -1.0;
    int elbow_index = 0;
    const double x1 = 0, y1 = k_dist[0];
    const double x2 = N - 1, y2 = k_dist.back();
    
    for (int i = 1; i < N - 1; ++i) {
        const double px = i, py = k_dist[i];
        const double dist = std::abs((y2 - y1) * px - (x2 - x1) * py + x2 * y1 - y2 * x1) / 
                            std::sqrt(std::pow(y2 - y1, 2) + std::pow(x2 - x1, 2));
        if (dist > max_dist_from_line) {
            max_dist_from_line = dist;
            elbow_index = i;
        }
    }
    
    double elbow_epsilon = k_dist[elbow_index];

    if (elbow_epsilon > 1e-9) {
        epsilons_to_run["_epsElbow"] = elbow_epsilon;
    }
    
    return epsilons_to_run;
}

/*
ClusteringResult ClusteringFeatureCalculator::run_dbscan_and_extract(const arma::mat& data, double epsilon, size_t minPoints,
                                                       const std::string& eps_suffix, FeatureSet& features) const {
    const int N = data.n_cols;

    // --- Execução do DBSCAN ---
    mlpack::DBSCAN<> dbscan_algorithm(epsilon, minPoints);
    arma::Row<size_t> assignments;
    const size_t num_clusters_from_mlpack = dbscan_algorithm.Cluster(data, assignments);

    // --- Análise dos Resultados ---
    ClusteringResult result;
    for (size_t i = 0; i < assignments.n_elem; ++i) {
        if (assignments[i] == ClusteringResult::OUTLIER_ID) {
        result.outlier_indices.push_back(i); // Guarda o índice do outlier
        } else {
        result.points_in_cluster[assignments[i]].push_back(i); // Guarda o índice do ponto no seu cluster
        }
    }
     // Agora, derivamos as contagens a partir da struct 'result'
    const size_t num_outliers = result.outlier_indices.size();
    const size_t num_clusters = result.points_in_cluster.size();

    // --- Extração e Salvamento das Features ---
    // CB1: Cluster Count
    features["CB1.1_num_clusters" + eps_suffix] = static_cast<double>(num_clusters);
    features["CB1.2_frac_clusters" + eps_suffix] = static_cast<double>(num_clusters) / N;

    // CB2: Node Classification Proportions
    features["CB2.3_frac_outlier_points" + eps_suffix] = static_cast<double>(num_outliers) / N;
    features["CB2.4_total_outlier_points" + eps_suffix] = static_cast<double>(num_outliers);
    features["CB2.1_frac_core_points" + eps_suffix] = -1.0; // Placeholder
    features["CB2.2_frac_border_points" + eps_suffix] = -1.0; // Placeholder

    if (num_clusters > 0) {
        // CB3: Distribution of Cluster Sizes (5-Descriptor Bundle)
        std::vector<double> cluster_sizes;
        cluster_sizes.reserve(num_clusters);
        for (const auto& pair : result.points_in_cluster) {
            cluster_sizes.push_back(static_cast<double>(pair.second.size())); // Usa o tamanho do vetor de pontos
        }
        
        auto stats_sizes = StatisticsCalculator::calculateAll(cluster_sizes);
        features["CB3.1_mean_cluster_size" + eps_suffix] = stats_sizes.mean;
        features["CB3.1_stddev_cluster_size" + eps_suffix] = stats_sizes.std_dev;
        features["CB3.1_min_cluster_size" + eps_suffix] = stats_sizes.min;
        features["CB3.1_max_cluster_size" + eps_suffix] = stats_sizes.max;
        features["CB3.1_median_cluster_size" + eps_suffix] = stats_sizes.median;

        // Placeholders para CB4 e CB5
        //features["CB4.1_mean_cluster_reach" + eps_suffix] = -1.0;
        //features["CB5.1_mean_intra_cluster_dist" + eps_suffix] = -1.0;
    }

    // Placeholder para CB6
    //features["CB6.1_mean_silhouette_coeff" + eps_suffix] = -1.0;
    return result;
}
*/

// ==========================================================================
// ARQUIVO: cvrp-feature-extractor/extractors/ClusteringFeatureCalculator.cpp
// FUNÇÃO A SER SUBSTITUÍDA: run_dbscan_and_extract
// ==========================================================================

ClusteringResult ClusteringFeatureCalculator::run_dbscan_and_extract(const arma::mat& data, double epsilon, size_t minPoints,
                                                       const std::string& eps_suffix, FeatureSet& features) const {
    const int N = data.n_cols;

    // --- Execução do DBSCAN ---
    mlpack::DBSCAN<> dbscan_algorithm(epsilon, minPoints);
    arma::Row<size_t> assignments;
    const size_t num_clusters_from_mlpack = dbscan_algorithm.Cluster(data, assignments);

    // --- Análise dos Resultados e Contagem Manual de Core/Border ---
    ClusteringResult result;
    size_t num_core = 0;
    size_t num_border = 0;
    
    // NOTA: O mlpack pode usar SIZE_MAX para outliers dependendo da versão, 
    // mas sua struct usa um ID específico. Vamos garantir a consistência.
    
    for (size_t i = 0; i < assignments.n_elem; ++i) {
        // Se for Outlier
        if (assignments[i] == ClusteringResult::OUTLIER_ID || assignments[i] == SIZE_MAX) {
            result.outlier_indices.push_back(i); 
        } 
        else {
            // Se pertence a um cluster, guardamos no mapa
            result.points_in_cluster[assignments[i]].push_back(i);

            // --- LÓGICA DE CORREÇÃO (CORE vs BORDER) ---
            // O DBSCAN já definiu os clusters. Agora precisamos classificar o ponto.
            // Definição:
            // Core Point: Tem >= minPoints vizinhos dentro do raio epsilon (incluindo ele mesmo).
            // Border Point: Pertence a um cluster (não é outlier), mas tem < minPoints vizinhos.
            
            size_t neighbors_count = 0;
            // Loop O(N) para contar vizinhos. Complexidade total O(N^2), aceitável para N ~ 1000.
            for (size_t j = 0; j < assignments.n_elem; ++j) {
                // Otimização: Distância Euclidiana ao quadrado evita raiz quadrada, 
                // mas mlpack usa euclidiana normal, então vamos manter consistência com arma::norm.
                if (arma::norm(data.col(i) - data.col(j)) <= epsilon) {
                    neighbors_count++;
                    // Pequena otimização: se já passou de minPoints, já sabemos que é Core.
                    if (neighbors_count >= minPoints) break; 
                }
            }

            if (neighbors_count >= minPoints) {
                num_core++;
            } else {
                num_border++;
            }
        }
    }

    const size_t num_outliers = result.outlier_indices.size();
    const size_t num_clusters = result.points_in_cluster.size();

    // --- Extração e Salvamento das Features ---
    
    // CB1: Cluster Count
    features["CB1.1_num_clusters" + eps_suffix] = static_cast<double>(num_clusters);
    features["CB1.2_frac_clusters" + eps_suffix] = static_cast<double>(num_clusters) / N;

    // CB2: Node Classification Proportions
    // Outliers
    features["CB2.3_frac_outlier_points" + eps_suffix] = static_cast<double>(num_outliers) / N;
    features["CB2.4_total_outlier_points" + eps_suffix] = static_cast<double>(num_outliers);
    
    // --- CORREÇÃO APLICADA AQUI ---
    // Agora salvamos os valores calculados em vez de -1.0
    features["CB2.1_frac_core_points" + eps_suffix] = static_cast<double>(num_core) / N;
    features["CB2.2_frac_border_points" + eps_suffix] = static_cast<double>(num_border) / N;
    // ------------------------------

    if (num_clusters > 0) {
        // CB3: Distribution of Cluster Sizes (5-Descriptor Bundle)
        std::vector<double> cluster_sizes;
        cluster_sizes.reserve(num_clusters);
        for (const auto& pair : result.points_in_cluster) {
            cluster_sizes.push_back(static_cast<double>(pair.second.size()));
        }
        
        auto stats_sizes = StatisticsCalculator::calculateAll(cluster_sizes);
        features["CB3.1_mean_cluster_size" + eps_suffix] = stats_sizes.mean;
        features["CB3.1_stddev_cluster_size" + eps_suffix] = stats_sizes.std_dev;
        features["CB3.1_min_cluster_size" + eps_suffix] = stats_sizes.min;
        features["CB3.1_max_cluster_size" + eps_suffix] = stats_sizes.max;
        features["CB3.1_median_cluster_size" + eps_suffix] = stats_sizes.median;
    }

    return result;
}