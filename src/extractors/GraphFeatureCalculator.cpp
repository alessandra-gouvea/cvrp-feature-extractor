#include <cvrp-feature-extractor/extractors/GraphFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>

// Includes da Boost Graph Library (BGL) que vamos precisar
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp> 
#include <boost/graph/dijkstra_shortest_paths.hpp> 

#include <vector>
#include <cmath> // Para std::hypot
#include <map>
#include <numeric>
#include <algorithm>

namespace {
    double euclidean_distance(const Node* a, const Node* b) {
        if (!a || !b) return 0.0;
        return std::hypot(a->getX() - b->getX(), a->getY() - b->getY());
    }

    std::vector<double> get_component_sizes(const std::vector<int>& component_map, int num_components) {
        std::map<int, double> component_sizes_map;
        for (int component_id : component_map) {
            component_sizes_map[component_id]++;
        }
        std::vector<double> sizes;
        sizes.reserve(num_components);
        for (auto const& [id, size] : component_sizes_map) {
            sizes.push_back(size);
        }
        return sizes;
    }
}

void GraphFeatureCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    // Não calcula features para instâncias com menos de 2 nós.
    const int N = problem.getDimension();
    if (N < 2) return;
    calculate_gb1_vertex_cost_features(problem, features);
    calculate_gb2_mbc_features(problem, features); 
    calculate_gb3_mst_features(problem, features);

    calculate_gb4_1_local_neighborhood_features(problem, features);
    if (N > 7) {
        calculate_gb4_2_dknng_features(problem, features);
    }
}

void GraphFeatureCalculator::calculate_gb1_vertex_cost_features(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    if (N < 2) return;

    // Converte os nós para um vetor para acesso rápido por índice
    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    for (const auto& pair : problem.getAllNodes()) {
       // nodes_by_index.push_back(pair.second->get());
       nodes_by_index.push_back(pair.second.get()); 
    }

    // Vetores para armazenar os custos de cada vértice
    std::vector<double> mean_vertex_costs;
    std::vector<double> median_vertex_costs;
    mean_vertex_costs.reserve(N);
    median_vertex_costs.reserve(N);

    // Itera sobre cada nó para calcular seu custo
    for (int i = 0; i < N; ++i) {
        std::vector<double> incident_costs;
        incident_costs.reserve(N - 1);
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
            incident_costs.push_back(euclidean_distance(nodes_by_index[i], nodes_by_index[j]));
        }
        
        // Calcula o custo médio para GB1.1
        if (!incident_costs.empty()) {
            mean_vertex_costs.push_back(std::accumulate(incident_costs.begin(), incident_costs.end(), 0.0) / incident_costs.size());
        }

        // Calcula o custo mediano para GB1.2
        if (!incident_costs.empty()) {
            // A mediana requer ordenação
            std::sort(incident_costs.begin(), incident_costs.end());
            median_vertex_costs.push_back(incident_costs[incident_costs.size() / 2]);
        }
    }
    
    // --- Salva as features de GB1.1: Mean Vertex Cost Distribution ---
    if (!mean_vertex_costs.empty()) {
        auto stats_mean = StatisticsCalculator::calculateAll(mean_vertex_costs);
        features["GB1.1_min_mean_vcost"] = stats_mean.min;
        features["GB1.1_max_mean_vcost"] = stats_mean.max;
        features["GB1.1_mean_mean_vcost"] = stats_mean.mean;
        features["GB1.1_median_mean_vcost"] = stats_mean.median;
        features["GB1.1_sum_mean_vcost"] = std::accumulate(mean_vertex_costs.begin(), mean_vertex_costs.end(), 0.0);
    }

    // --- Salva as features de GB1.2: Median Vertex Cost Distribution ---
    if (!median_vertex_costs.empty()) {
        auto stats_median = StatisticsCalculator::calculateAll(median_vertex_costs);
        features["GB1.2_min_median_vcost"] = stats_median.min;
        features["GB1.2_median_median_vcost"] = stats_median.median;
        features["GB1.2_max_median_vcost"] = stats_median.max;
    }
}

void GraphFeatureCalculator::calculate_gb3_mst_features(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    if (N < 2) return;

    // --- Parte 1: Construção do Grafo e Coleta de Dados Iniciais ---
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                      boost::no_property,
                                      boost::property<boost::edge_weight_t, double>>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;

    Graph g(N);
    std::vector<const Node*> nodes_by_index;
    std::map<int, Vertex> node_id_to_vertex_map;
    
    nodes_by_index.reserve(N);
    int vertex_idx = 0;
    for (const auto& pair : problem.getAllNodes()) {
        nodes_by_index.push_back(pair.second.get());
        node_id_to_vertex_map[pair.second->getId()] = vertex_idx++;
    }

    double total_pairwise_dist = 0.0; 
    for (int j = 0; j < N; ++j) {
        for (int k = j + 1; k < N; ++k) {
            double dist = euclidean_distance(nodes_by_index[j], nodes_by_index[k]);
            boost::add_edge(j, k, dist, g);
            total_pairwise_dist += dist;
        }
    }

    std::vector<Edge> mst_edges;
    boost::kruskal_minimum_spanning_tree(g, std::back_inserter(mst_edges));

    // --- Parte 2: Extrair Features da MST (Apenas valores brutos) ---
    
    // GB3.1: MST Edge Cost Distribution
    std::vector<double> mst_edge_costs;
    mst_edge_costs.reserve(mst_edges.size());
    for (const auto& edge : mst_edges) {
        mst_edge_costs.push_back(get(boost::edge_weight, g, edge));
    }

    if (!mst_edge_costs.empty()) {
        auto stats = StatisticsCalculator::calculateAll(mst_edge_costs);
        auto modal_stats = StatisticsCalculator::analyzeModes(mst_edge_costs);
        features["GB3.1_mean_mst_cost"] = stats.mean;
        features["GB3.1_stddev_mst_cost"] = stats.std_dev;
        features["GB3.1_min_mst_cost"] = stats.min;
        features["GB3.1_max_mst_cost"] = stats.max;
        features["GB3.1_median_mst_cost"] = stats.median;
        features["GB3.1_skew_mst_cost"] = stats.skewness;
        features["GB3.1_kurt_mst_cost"] = stats.kurtosis;
        features["GB3.1_cv_mst_cost"] = stats.cv;
        features["GB3.1_q1_mst_cost"] = stats.q1;
        features["GB3.1_q3_mst_cost"] = stats.q3;
        features["GB3.1_num_modes_mst_cost"] = static_cast<double>(modal_stats.num_modes);
    }

    // GB3.2: MST Node Degree Distribution
    std::vector<double> mst_node_degrees(N, 0.0);
    for (const auto& edge : mst_edges) {
        mst_node_degrees[source(edge, g)]++;
        mst_node_degrees[target(edge, g)]++;
    }
    auto stats_degrees = StatisticsCalculator::calculateAll(mst_node_degrees);
    features["GB3.2_mean_mst_node_degree"] = stats_degrees.mean;
    features["GB3.2_stddev_mst_node_degree"] = stats_degrees.std_dev;
    features["GB3.2_min_mst_node_degree"] = stats_degrees.min;
    features["GB3.2_max_mst_node_degree"] = stats_degrees.max;
    features["GB3.2_median_mst_node_degree"] = stats_degrees.median;

    // GB3.4: MST Node Depth from Depot Distribution
    const Node* depot_node = problem.getDepot();
    if (depot_node) {
        Vertex depot_vertex = node_id_to_vertex_map.at(depot_node->getId());
        
        Graph mst_graph(N);
        for(const auto& edge : mst_edges) {
            boost::add_edge(source(edge, g), target(edge, g), mst_graph);
        }

        std::vector<double> depths(N, 0.0);
        boost::breadth_first_search(mst_graph, depot_vertex, 
            boost::visitor(
                boost::make_bfs_visitor(
                    boost::record_distances(depths.data(), boost::on_tree_edge())
                )
            )
        );
        
        auto stats_depths = StatisticsCalculator::calculateAll(depths);
        features["GB3.4_mean_mst_depot_depth"] = stats_depths.mean;
        features["GB3.4_stddev_mst_depot_depth"] = stats_depths.std_dev;
        features["GB3.4_min_mst_depot_depth"] = stats_depths.min;
        features["GB3.4_max_mst_depot_depth"] = stats_depths.max;
        features["GB3.4_median_mst_depot_depth"] = stats_depths.median;
    }
    
    // GB3.5: Normalized Sum of MST Edge Costs
    // --- CORREÇÃO AQUI ---
    double total_mst_cost = std::accumulate(mst_edge_costs.begin(), mst_edge_costs.end(), 0.0);
    if (total_pairwise_dist > 1e-9) {
        features["GB3.5_norm_sum_mst_costs"] = total_mst_cost / total_pairwise_dist;
    } else {
        features["GB3.5_norm_sum_mst_costs"] = 0.0;
    }

    // Placeholder para a feature GB3.3
    //features["GB3.3_mean_mst_generic_depth"] = -1.0;
}

//antiga calculate_gb_nn1_features
void GraphFeatureCalculator::calculate_gb4_1_local_neighborhood_features(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    // Requer pelo menos 4 nós para ter um 3º vizinho (1 nó de origem + 3 vizinhos).
    if (N < 4) return;

    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    for (const auto& pair : problem.getAllNodes()) {
        nodes_by_index.push_back(pair.second.get());
    }

    // --- Passo 1: Pré-calcular e ordenar as distâncias BRUTAS para cada nó ---
    std::vector<std::vector<double>> all_sorted_dists(N);
    for (int i = 0; i < N; ++i) {
        all_sorted_dists[i].reserve(N - 1);
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
            all_sorted_dists[i].push_back(euclidean_distance(nodes_by_index[i], nodes_by_index[j]));
        }
        std::sort(all_sorted_dists[i].begin(), all_sorted_dists[i].end());
    }

    // --- Passo 2: Extrair features GB4.1.1 (k-Nearest Neighbors) ---
    // Apenas os valores brutos (não normalizados) são calculados aqui.
    for (int k = 1; k <= 3; ++k) {
        std::vector<double> knn_distances;
        knn_distances.reserve(N);
        for (int i = 0; i < N; ++i) {
            knn_distances.push_back(all_sorted_dists[i][k - 1]);
        }
        
        std::string k_str = std::to_string(k);
        std::string prefix = "GB4.1.1_" + k_str + "nn_";

        if (k == 1) { // O catálogo pede o 11-Descriptor Bundle para k=1.
            auto stats = StatisticsCalculator::calculateAll(knn_distances);
            auto modal_stats = StatisticsCalculator::analyzeModes(knn_distances);
            features[prefix + "mean_dist"] = stats.mean;
            features[prefix + "stddev_dist"] = stats.std_dev;
            features[prefix + "min_dist"] = stats.min;
            features[prefix + "max_dist"] = stats.max;
            features[prefix + "median_dist"] = stats.median;
            features[prefix + "skew_dist"] = stats.skewness;
            features[prefix + "kurt_dist"] = stats.kurtosis;
            features[prefix + "cv_dist"] = stats.cv;
            features[prefix + "q1_dist"] = stats.q1;
            features[prefix + "q3_dist"] = stats.q3;
            features[prefix + "num_modes_dist"] = static_cast<double>(modal_stats.num_modes);
        } else { // O catálogo pede Média e Desvio Padrão para k=2 e k=3.
            auto stats = StatisticsCalculator::calculateAll(knn_distances);
            features[prefix + "mean_dist"] = stats.mean;
            features[prefix + "stddev_dist"] = stats.std_dev;
        }
    }

    // --- Passo 3: Extrair features GB4.1.2 (k-Farthest Neighbors) ---
    // Apenas os valores brutos (não normalizados) são calculados aqui.
    for (int k = 1; k <= 3; ++k) {
        std::vector<double> kfn_distances;
        kfn_distances.reserve(N);
        for (int i = 0; i < N; ++i) {
            kfn_distances.push_back(all_sorted_dists[i][(N - 1) - k]);
        }
        auto stats = StatisticsCalculator::calculateAll(kfn_distances);
        std::string k_str = std::to_string(k);
        std::string prefix = "GB4.1.2_" + k_str + "fn_";
        features[prefix + "mean_dist"] = stats.mean;
        features[prefix + "stddev_dist"] = stats.std_dev;
    }

    // A parte que calculava a normalização local ("FN.*") foi completamente removida.
    // Esta responsabilidade agora pertence exclusivamente à NormalizationEngine.
}

//antiga calculate_gb_nn2_features
void GraphFeatureCalculator::calculate_gb4_2_dknng_features(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    // A verificação de N > k já foi feita no orquestrador, mas é bom ser seguro.
    if (N <= 1) return;

    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    for (const auto& pair : problem.getAllNodes()) {
        nodes_by_index.push_back(pair.second.get());
    }

    // --- Definimos um Grafo Dirigido para esta função ---
    using DirectedGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
    using Vertex = boost::graph_traits<DirectedGraph>::vertex_descriptor;

    // Lógica original para seleção de k
    std::set<int> k_values_set = {3, 5, 7};
    k_values_set.insert(static_cast<int>(std::floor(std::pow(N, 1.0/3.0))));
    k_values_set.insert(static_cast<int>(std::floor(std::pow(N, 1.0/2.0))));
    const std::vector<int> k_values(k_values_set.begin(), k_values_set.end());

    // --- Loop principal sobre os valores de k ---
    for (int k : k_values) {
        if (N <= k) continue; // Pula se não houver nós suficientes

        // 1. Construir o Grafo Dirigido k-NN
        DirectedGraph g_knn(N);
        for (int i = 0; i < N; ++i) {
            std::vector<std::pair<double, int>> dists_to_others;
            dists_to_others.reserve(N - 1);
            for (int j = 0; j < N; ++j) {
                if (i == j) continue;
                dists_to_others.push_back({euclidean_distance(nodes_by_index[i], nodes_by_index[j]), j});
            }
            // Ordena parcialmente para encontrar os k vizinhos mais próximos de forma eficiente
            std::partial_sort(dists_to_others.begin(), dists_to_others.begin() + k, dists_to_others.end());

            // Adiciona as k arestas dirigidas
            for (int neighbor_idx = 0; neighbor_idx < k; ++neighbor_idx) {
                boost::add_edge(i, dists_to_others[neighbor_idx].second, g_knn);
            }
        }
        
        // --- Análises no Grafo g_knn ---
        std::string k_suffix = "_k" + std::to_string(k);
        
        // --- GB4.2.1: Node Input Degree Distribution (11-Descriptor Bundle) ---
        std::vector<double> in_degrees;
        in_degrees.reserve(N);
        for (int i = 0; i < N; ++i) in_degrees.push_back(boost::in_degree(i, g_knn));
        auto stats_indegree = StatisticsCalculator::calculateAll(in_degrees);
        auto modal_stats_indegree = StatisticsCalculator::analyzeModes(in_degrees);
        features["GB4.2.1_mean_indegree" + k_suffix] = stats_indegree.mean;
        features["GB4.2.1_stddev_indegree" + k_suffix] = stats_indegree.std_dev;
        features["GB4.2.1_min_indegree" + k_suffix] = stats_indegree.min;
        features["GB4.2.1_max_indegree" + k_suffix] = stats_indegree.max;
        features["GB4.2.1_median_indegree" + k_suffix] = stats_indegree.median;
        features["GB4.2.1_skew_indegree" + k_suffix] = stats_indegree.skewness;
        features["GB4.2.1_kurt_indegree" + k_suffix] = stats_indegree.kurtosis;
        features["GB4.2.1_cv_indegree" + k_suffix] = stats_indegree.cv;
        features["GB4.2.1_q1_indegree" + k_suffix] = stats_indegree.q1;
        features["GB4.2.1_q3_indegree" + k_suffix] = stats_indegree.q3;
        features["GB4.2.1_num_modes_indegree" + k_suffix] = static_cast<double>(modal_stats_indegree.num_modes);

        // --- GB4.2.2 & GB4.2.3: Strongly Connected Components (SCC) ---
        std::vector<int> scc_map(N);
        int num_scc = boost::strong_components(g_knn, boost::make_iterator_property_map(scc_map.begin(), get(boost::vertex_index, g_knn)));
        features["GB4.2.2_num_scc" + k_suffix] = static_cast<double>(num_scc);

        if (num_scc > 0) {
            std::vector<double> scc_sizes = get_component_sizes(scc_map, num_scc);
            auto stats_scc = StatisticsCalculator::calculateAll(scc_sizes);
            auto modal_scc = StatisticsCalculator::analyzeModes(scc_sizes);
            
            features["GB4.2.3_mean_scc_size" + k_suffix] = stats_scc.mean;
            features["GB4.2.3_stddev_scc_size" + k_suffix] = stats_scc.std_dev;
            features["GB4.2.3_min_scc_size" + k_suffix] = stats_scc.min;
            features["GB4.2.3_max_scc_size" + k_suffix] = stats_scc.max;
            features["GB4.2.3_median_scc_size" + k_suffix] = stats_scc.median;
            features["GB4.2.3_skew_scc_size" + k_suffix] = stats_scc.skewness;
            features["GB4.2.3_kurt_scc_size" + k_suffix] = stats_scc.kurtosis;
            features["GB4.2.3_cv_scc_size" + k_suffix] = stats_scc.cv;
            features["GB4.2.3_q1_scc_size" + k_suffix] = stats_scc.q1;
            features["GB4.2.3_q3_scc_size" + k_suffix] = stats_scc.q3;
            features["GB4.2.3_num_modes_scc_size" + k_suffix] = static_cast<double>(modal_scc.num_modes);
        }

        // --- GB4.2.4 & GB4.2.5: Weakly Connected Components (WCC) ---
        std::vector<int> wcc_map(N);
        int num_wcc = boost::connected_components(g_knn, &wcc_map[0]);
        features["GB4.2.4_num_wcc" + k_suffix] = static_cast<double>(num_wcc);

        if (num_wcc > 0) {
            std::vector<double> wcc_sizes = get_component_sizes(wcc_map, num_wcc);
            auto stats_wcc = StatisticsCalculator::calculateAll(wcc_sizes);
            auto modal_wcc = StatisticsCalculator::analyzeModes(wcc_sizes);

            features["GB4.2.5_mean_wcc_size" + k_suffix] = stats_wcc.mean;
            features["GB4.2.5_stddev_wcc_size" + k_suffix] = stats_wcc.std_dev;
            features["GB4.2.5_min_wcc_size" + k_suffix] = stats_wcc.min;
            features["GB4.2.5_max_wcc_size" + k_suffix] = stats_wcc.max;
            features["GB4.2.5_median_wcc_size" + k_suffix] = stats_wcc.median;
            features["GB4.2.5_skew_wcc_size" + k_suffix] = stats_wcc.skewness;
            features["GB4.2.5_kurt_wcc_size" + k_suffix] = stats_wcc.kurtosis;
            features["GB4.2.5_cv_wcc_size" + k_suffix] = stats_wcc.cv;
            features["GB4.2.5_q1_wcc_size" + k_suffix] = stats_wcc.q1;
            features["GB4.2.5_q3_wcc_size" + k_suffix] = stats_wcc.q3;
            features["GB4.2.5_num_modes_wcc_size" + k_suffix] = static_cast<double>(modal_wcc.num_modes);
        }
        
        // --- GB4.2.6: Ratio of SCCs to WCCs ---
        if (num_wcc > 0) {
            features["GB4.2.6_ratio_scc_wcc" + k_suffix] = static_cast<double>(num_scc) / num_wcc;
        }
    }
}

void GraphFeatureCalculator::calculate_gb2_mbc_features(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    if (N < 2) return;

    // --- Tipos da BGL ---
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                      boost::no_property,
                                      boost::property<boost::edge_weight_t, double>>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;

    // --- Passo 1: Construir Grafo Completo e Calcular MST (código reutilizado) ---
    Graph g(N);
    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    for (const auto& pair : problem.getAllNodes()) {
        nodes_by_index.push_back(pair.second.get());
    }

    for (int j = 0; j < N; ++j) {
        for (int k = j + 1; k < N; ++k) {
            boost::add_edge(j, k, euclidean_distance(nodes_by_index[j], nodes_by_index[k]), g);
        }
    }

    std::vector<Edge> mst_edges;
    boost::kruskal_minimum_spanning_tree(g, std::back_inserter(mst_edges));

    // --- Passo 2: Construir um Grafo apenas com as arestas da MST ---
    Graph mst_graph(N);
    for (const auto& edge : mst_edges) {
        Vertex u = boost::source(edge, g);
        Vertex v = boost::target(edge, g);
        double weight = get(boost::edge_weight, g, edge);
        boost::add_edge(u, v, weight, mst_graph);
    }

    // --- Passo 3: Calcular o MBC para cada par de nós ---
    std::vector<double> all_mbc_costs;
    std::vector<Vertex> predecessors(N);
    std::vector<double> distances(N);

    for (int i = 0; i < N; ++i) {
        // Encontra o caminho de todos os nós para 'i' na MST
        boost::dijkstra_shortest_paths(mst_graph, i, 
            boost::predecessor_map(boost::make_iterator_property_map(predecessors.begin(), get(boost::vertex_index, mst_graph)))
        );

        // Para cada nó 'j', traça o caminho de volta para 'i' e encontra a aresta mais longa
        for (int j = i + 1; j < N; ++j) {
            double max_edge_in_path = 0.0;
            Vertex current = j;
            while (current != i) {
                Vertex pred = predecessors[current];
                // Encontra a aresta entre 'current' e seu predecessor
                auto edge_pair = boost::edge(pred, current, mst_graph);
                if (edge_pair.second) { // Se a aresta existe
                    double edge_weight = get(boost::edge_weight, mst_graph, edge_pair.first);
                    if (edge_weight > max_edge_in_path) {
                        max_edge_in_path = edge_weight;
                    }
                }
                current = pred;
            }
            all_mbc_costs.push_back(max_edge_in_path);
        }
    }
    
    // --- Passo 4: Calcular as estatísticas sobre os MBCs ---
    if (!all_mbc_costs.empty()) {
        auto stats = StatisticsCalculator::calculateAll(all_mbc_costs);
        features["GB2.1_mean_mbc"] = stats.mean;
        features["GB2.1_stddev_mbc"] = stats.std_dev;
        features["GB2.1_min_mbc"] = stats.min;
        features["GB2.1_max_mbc"] = stats.max;
        features["GB2.1_median_mbc"] = stats.median;
    }
}