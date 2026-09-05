#include <cvrp-feature-extractor/extractors/ProbingFeatureCalculator.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h> 

// --- Includes do Google OR-Tools ---
#include "ortools/constraint_solver/routing.h"
#include "ortools/constraint_solver/routing_enums.pb.h"
#include "ortools/constraint_solver/routing_index_manager.h"
#include "ortools/constraint_solver/routing_parameters.h"
#include <armadillo>

#include <vector>
#include <cmath>
#include <numeric>

// Namespace para facilitar o uso das classes do OR-Tools
namespace or_tools = operations_research;

// Função auxiliar para calcular distância euclidiana
namespace {
    double euclidean_distance(const Node* a, const Node* b) {
        if (!a || !b) return 0.0;
        return std::hypot(a->getX() - b->getX(), a->getY() - b->getY());
    }

    bool do_lines_intersect(const Node* p1, const Node* q1, const Node* p2, const Node* q2) {
        // Função para encontrar a orientação de um triplet ordenado (p, q, r)
        // 0 --> p, q e r são colineares
        // 1 --> sentido horário
        // 2 --> sentido anti-horário
        auto orientation = [](const Node* p, const Node* q, const Node* r) -> int {
            double val = (q->getY() - p->getY()) * (r->getX() - q->getX()) -
                         (q->getX() - p->getX()) * (r->getY() - q->getY());
            if (std::abs(val) < 1e-9) return 0; // Colinear
            return (val > 0) ? 1 : 2; // Horário ou Anti-horário
        };

        // Função para verificar se o ponto q está no segmento pr
        auto on_segment = [](const Node* p, const Node* q, const Node* r) {
            return (q->getX() <= std::max(p->getX(), r->getX()) && q->getX() >= std::min(p->getX(), r->getX()) &&
                    q->getY() <= std::max(p->getY(), r->getY()) && q->getY() >= std::min(p->getY(), r->getY()));
        };

        int o1 = orientation(p1, q1, p2);
        int o2 = orientation(p1, q1, q2);
        int o3 = orientation(p2, q2, p1);
        int o4 = orientation(p2, q2, q1);

        // Caso geral de interseção
        if (o1 != o2 && o3 != o4) return true;

        // Casos especiais de colinearidade
        if (o1 == 0 && on_segment(p1, p2, q1)) return true;
        if (o2 == 0 && on_segment(p1, q2, q1)) return true;
        if (o3 == 0 && on_segment(p2, p1, q2)) return true;
        if (o4 == 0 && on_segment(p2, q1, q2)) return true;

        return false;
    }
}

void ProbingFeatureCalculator::calculate(const CVRP& problem, FeatureSet& features) const {
    const int N = problem.getDimension();
    if (N < 2 || problem.getDepot() == nullptr) {
        return; // Pré-condições para a sondagem
    }

    // =========================================================================
    // --- Passo 1: Preparar os Dados para o OR-Tools ---
    // =========================================================================

    // O OR-Tools espera que o depósito esteja no índice 0. Criamos um mapeamento.
    std::vector<const Node*> nodes_by_index;
    nodes_by_index.reserve(N);
    
    const Node* depot = problem.getDepot();
    nodes_by_index.push_back(depot);
    for (const auto& pair : problem.getAllNodes()) {
        if (!pair.second->isDepot()) {
            nodes_by_index.push_back(pair.second.get());
        }
    }
    
    // O OR-Tools funciona melhor com custos/demandas inteiras.
    // Usamos um fator de escala para manter a precisão dos doubles.
    const int64_t scale_factor = 1000;

    // a) Matriz de distâncias
    std::vector<std::vector<int64_t>> distance_matrix(N, std::vector<int64_t>(N, 0));
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            distance_matrix[i][j] = static_cast<int64_t>(
                euclidean_distance(nodes_by_index[i], nodes_by_index[j]) * scale_factor
            );
        }
    }

    // b) Vetor de demandas
    std::vector<int64_t> demands;
    demands.reserve(N);
    for (const auto* node : nodes_by_index) {
        demands.push_back(node->getDemand());
    }

    // =========================================================================
    // --- Passo 2: Construir o Modelo de Roteamento ---
    // =========================================================================
    
    // Usamos N como um limite superior seguro para o número de veículos.
    const int num_vehicles = N; 
    const or_tools::RoutingIndexManager::NodeIndex depot_node_index{0};

    or_tools::RoutingIndexManager manager(N, num_vehicles, depot_node_index);
    or_tools::RoutingModel routing(manager);

    // =========================================================================
    // --- Passo 3: Definir as Dimensões (Custos e Restrições) ---
    // =========================================================================

    // a) Custo de Arco (Distância)
    const int transit_callback_index = routing.RegisterTransitCallback(
        [&distance_matrix, &manager](int64_t from_index, int64_t to_index) -> int64_t {
            int from_node = manager.IndexToNode(from_index).value();
            int to_node = manager.IndexToNode(to_index).value();
            return distance_matrix[from_node][to_node];
        }
    );
    routing.SetArcCostEvaluatorOfAllVehicles(transit_callback_index);

    // b) Restrição de Capacidade (Demanda)
    const int demand_callback_index = routing.RegisterUnaryTransitCallback(
        [&demands, &manager](int64_t from_index) -> int64_t {
            int from_node = manager.IndexToNode(from_index).value();
            return demands[from_node];
        }
    );
    routing.AddDimension(
        demand_callback_index, 
        0, // slack_max (sem tempo de espera no depósito)
        problem.getCapacity(), // vehicle_capacity
        true, // start_cumul_to_zero
        "Capacity"
    );

    // =========================================================================
    // --- Passo 4: Executar a SONDAGEM CONSTRUTIVA (para PT5.1) ---
    // =========================================================================
     // a) Configura os parâmetros para usar APENAS a heurística construtiva
    or_tools::RoutingSearchParameters construction_params = or_tools::DefaultRoutingSearchParameters();
    construction_params.set_first_solution_strategy(
        or_tools::FirstSolutionStrategy::PATH_CHEAPEST_ARC
    );
    // Desliga completamente a busca local para esta primeira execução
    construction_params.set_local_search_metaheuristic(
        or_tools::LocalSearchMetaheuristic::TABU_SEARCH // Pode ser qualquer uma, o tempo limite a impede de rodar
    );
    construction_params.mutable_time_limit()->set_seconds(0); // Garante que a busca local não tenha tempo de rodar
    // Uma alternativa mais explícita seria usar um filtro para aceitar apenas a primeira solução.
    
    // b) Executa o solver para obter a solução inicial
    const or_tools::Assignment* initial_solution = routing.SolveWithParameters(construction_params);

    // c) Salva a feature PT5.1
    if (initial_solution) {
        features["PT5.1_quality_after_construction"] = static_cast<double>(initial_solution->ObjectiveValue()) / scale_factor;
    } else {
        features["PT5.1_quality_after_construction"] = -1.0;
    }


    // =========================================================================
    // --- Passo 5: Executar a SONDAGEM COMPLETA (para as outras features) ---
    // =========================================================================
    
    // a) Configura os parâmetros para a busca completa (código que já tínhamos)
    or_tools::RoutingSearchParameters search_parameters = or_tools::DefaultRoutingSearchParameters();
    search_parameters.set_first_solution_strategy(
        or_tools::FirstSolutionStrategy::PATH_CHEAPEST_ARC
    );
    search_parameters.set_local_search_metaheuristic(
        or_tools::LocalSearchMetaheuristic::GUIDED_LOCAL_SEARCH
    );
    search_parameters.mutable_time_limit()->set_seconds(3);

    // b) Executa o solver
    const or_tools::Assignment* final_solution = routing.SolveWithParameters(search_parameters);

    // =========================================================================
    // --- Passo 6: Extrair as Features da Solução ---
    // =========================================================================
    if (final_solution) {
        // PT5.2: Qualidade da Solução após Busca Local
        features["PT5.2_solution_quality_after_LS"] = static_cast<double>(final_solution->ObjectiveValue()) / scale_factor;
        
        // --- Coleta de dados de todas as rotas usadas ---
        std::vector<double> route_loads, route_lengths, route_num_cust;
        std::vector<double> max_intra_route_edges, depot_conn_dists, route_spans, route_avg_radii;
        std::vector<arma::vec> route_centroids; // Para PT5.15

        for (int i = 0; i < num_vehicles; ++i) {
            if (!routing.IsVehicleUsed(*final_solution, i)) continue;

            std::vector<const Node*> route_nodes_ptr;
            std::vector<int64_t> route_node_indices;
            int64_t index = routing.Start(i);
            
            while (!routing.IsEnd(index)) {
                int node_idx = manager.IndexToNode(index).value();
                route_nodes_ptr.push_back(nodes_by_index[node_idx]);
                route_node_indices.push_back(node_idx);
                index = final_solution->Value(routing.NextVar(index));
            }
            // Adiciona o último nó (depósito final)
            route_nodes_ptr.push_back(nodes_by_index[manager.IndexToNode(index).value()]);
            
            // --- Cálculos por Rota ---
            if (route_nodes_ptr.size() > 2) { // Rota tem pelo menos um cliente
                route_num_cust.push_back(static_cast<double>(route_nodes_ptr.size() - 2));

                int64_t current_route_load = 0;
                int64_t current_route_length = 0;
                for (size_t j = 0; j < route_node_indices.size() -1; ++j) {
                    int from_node_idx = route_node_indices[j];
                    int to_node_idx = route_node_indices[j+1];
                    current_route_load += demands[from_node_idx];
                    current_route_length += distance_matrix[from_node_idx][to_node_idx];
                }
                route_loads.push_back(static_cast<double>(current_route_load));
                route_lengths.push_back(static_cast<double>(current_route_length) / scale_factor);

                // PT5.14: Distância de Conexão com Depósito
                const Node* first_cust = route_nodes_ptr[1];
                const Node* last_cust = route_nodes_ptr[route_nodes_ptr.size() - 2];
                depot_conn_dists.push_back((euclidean_distance(depot, first_cust) + euclidean_distance(last_cust, depot)) / 2.0);

                // PT5.13: Máxima Aresta Intra-Rota
                double max_edge = 0.0;
                for (size_t j = 1; j < route_nodes_ptr.size() - 2; ++j) {
                    max_edge = std::max(max_edge, euclidean_distance(route_nodes_ptr[j], route_nodes_ptr[j+1]));
                }
                max_intra_route_edges.push_back(max_edge);
                
                // PT5.16: Métricas de Forma da Rota
                arma::mat customer_points(2, route_num_cust.back());
                double max_dist_in_route = 0.0;
                for(size_t j = 0; j < route_num_cust.back(); ++j) {
                    const Node* cust_node = route_nodes_ptr[j + 1];
                    customer_points(0, j) = cust_node->getX();
                    customer_points(1, j) = cust_node->getY();
                    for (size_t l = j + 1; l < route_num_cust.back(); ++l) {
                        max_dist_in_route = std::max(max_dist_in_route, euclidean_distance(cust_node, route_nodes_ptr[l + 1]));
                    }
                }
                route_spans.push_back(max_dist_in_route);
                arma::vec centroid = arma::mean(customer_points, 1);
                route_centroids.push_back(centroid);
                double avg_radius = 0.0;
                for(size_t j = 0; j < customer_points.n_cols; ++j) {
                    avg_radius += arma::norm(customer_points.col(j) - centroid);
                }
                route_avg_radii.push_back(avg_radius / customer_points.n_cols);
            }
        }
        
        // --- Calcular e Salvar as Estatísticas Agregadas ---
        
        // PT5.17: Desequilíbrio do Tamanho das Rotas
        if (!route_num_cust.empty()) {
            auto stats = StatisticsCalculator::calculateAll(route_num_cust);
            features["PT5.17_route_size_imbalance_stdev"] = stats.std_dev;
        }

        // PT5.13: Distribuição da Máxima Aresta Intra-Rota
        if (!max_intra_route_edges.empty()) {
            auto stats = StatisticsCalculator::calculateAll(max_intra_route_edges);
            features["PT5.13_mean_max_intra_route_edge"] = stats.mean;
        }
        
        // PT5.14: Distribuição da Distância de Conexão com o Depósito
        if (!depot_conn_dists.empty()) {
            auto stats = StatisticsCalculator::calculateAll(depot_conn_dists);
            features["PT5.14_mean_depot_conn_dist"] = stats.mean;
        }

        // PT5.15: Distribuição das Distâncias Inter-Rotas (Centroides)
        if (route_centroids.size() > 1) {
            std::vector<double> inter_route_dists;
            for (size_t i = 0; i < route_centroids.size(); ++i) {
                for (size_t j = i + 1; j < route_centroids.size(); ++j) {
                    inter_route_dists.push_back(arma::norm(route_centroids[i] - route_centroids[j]));
                }
            }
            auto stats = StatisticsCalculator::calculateAll(inter_route_dists);
            features["PT5.15_mean_inter_route_dist"] = stats.mean;
        }
        
        // PT5.16: Distribuição das Métricas de Forma da Rota
        if (!route_spans.empty()) {
            auto stats = StatisticsCalculator::calculateAll(route_spans);
            features["PT5.16_mean_route_span"] = stats.mean;
        }
        if (!route_avg_radii.empty()) {
            auto stats = StatisticsCalculator::calculateAll(route_avg_radii);
            features["PT5.16_mean_route_avg_radius"] = stats.mean;
        }

         double total_intersections = 0;
        for (int i = 0; i < num_vehicles; ++i) {
            if (!routing.IsVehicleUsed(*final_solution, i)) continue;
            
            // Reconstrói a rota para este veículo
            std::vector<const Node*> route_nodes_ptr;
            int64_t index = routing.Start(i);
            while (!routing.IsEnd(index)) {
                route_nodes_ptr.push_back(nodes_by_index[manager.IndexToNode(index).value()]);
                index = final_solution->Value(routing.NextVar(index));
            }
            route_nodes_ptr.push_back(nodes_by_index[manager.IndexToNode(index).value()]);

            // Uma rota precisa de pelo menos 4 nós (2 arestas) para ter uma interseção.
            if (route_nodes_ptr.size() < 4) continue;
            
            // Compara cada par de arestas não adjacentes na rota
            for (size_t j = 0; j < route_nodes_ptr.size() - 2; ++j) {
                for (size_t l = j + 2; l < route_nodes_ptr.size() - 1; ++l) {
                    // Evita comparar a última aresta (que volta ao início) com a primeira
                    if (j == 0 && l == route_nodes_ptr.size() - 2) continue;
                    
                    const Node* p1 = route_nodes_ptr[j];
                    const Node* q1 = route_nodes_ptr[j + 1];
                    const Node* p2 = route_nodes_ptr[l];
                    const Node* q2 = route_nodes_ptr[l + 1];
                    
                    if (do_lines_intersect(p1, q1, p2, q2)) {
                        total_intersections++;
                    }
                }
            }
        }
        features["PT5.8_total_intersections"] = total_intersections;

    } else {
        //features["PT5.2_solution_quality_after_LS"] = -1.0;
    }

    // Adiciona Placeholders para as features PT5 que exigem protocolos mais complexos
    //features["PT5.3_mean_dist_local_minima"] = -1.0; // Requer múltiplos runs
    //features["PT5.4_mean_improvement_per_LS_step"] = -1.0; // Requer search monitor
    //features["PT5.5_mean_LS_steps_to_min"] = -1.0; // Requer search monitor
    //features["PT5.7_mean_backbone_freq"] = -1.0; // Requer múltiplos runs
}