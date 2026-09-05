#ifndef CLUSTERINGFEATURECALCULATOR_H_
#define CLUSTERINGFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <cvrp-feature-extractor/core/SharedTypes.h> 
#include <armadillo>
#include <map>
#include <string>

/**
 * @class ClusteringFeatureCalculator
 * @brief Especialista dedicado a calcular features de clustering (CB) usando DBSCAN.
 *
 * Esta classe usa a biblioteca mlpack para executar o algoritmo DBSCAN nos
 * dados da instância e extrair características sobre os clusters encontrados.
 */
class ClusteringFeatureCalculator {
public:
    ClusteringFeatureCalculator() = default;

    /**
     * @brief Calcula todas as features CB e as adiciona ao mapa de resultados.
     * @param problem A instância CVRP a ser analisada.
     * @param features O mapa onde as novas features serão inseridas.
     */
    ClusteringResult calculate(const CVRP& problem, FeatureSet& features) const;

private:
    /**
     * @brief Calcula um conjunto de valores de epsilon robustos para a análise.
     * @param data A matriz de dados dos nós (formato Armadillo).
     * @return Um mapa de sufixos de feature para os valores de epsilon calculados.
     */
    std::map<std::string, double> calculate_epsilons(const arma::mat& data) const;

    /**
     * @brief Executa uma única rodada do DBSCAN e extrai as features correspondentes.
     * @param data A matriz de dados dos nós.
     * @param epsilon O valor de epsilon a ser usado para esta execução.
     * @param minPoints O número mínimo de pontos para formar um cluster.
     * @param eps_suffix O sufixo a ser anexado aos nomes das features (ex: "_epsElbow").
     * @param features O mapa de features onde os resultados serão inseridos.
     */
    ClusteringResult run_dbscan_and_extract(const arma::mat& data, double epsilon, size_t minPoints,
                                const std::string& eps_suffix, FeatureSet& features) const;
    
    /**
     * @brief Calcula e salva as features de distância intra-cluster (CB5).
     * @param data A matriz de dados dos nós.
     * @param assignments O vetor de atribuição de cluster para cada ponto.
     * @param num_clusters O número total de clusters encontrados.
     * @param eps_suffix O sufixo da feature.
     * @param features O mapa para inserir os resultados.
     */
    void calculate_cb5_intra_cluster_distances(const arma::mat& data,
                                               const arma::Row<size_t>& assignments,
                                               size_t num_clusters,
                                               const std::string& eps_suffix,
                                               FeatureSet& features) const;

        /**
     * @brief Calcula e salva o Coeficiente de Silhouette médio (CB6).
     * @param data A matriz de dados dos nós.
     * @param assignments O vetor de atribuição de cluster para cada ponto.
     * @param num_clusters O número total de clusters encontrados.
     * @param eps_suffix O sufixo da feature.
     * @param features O mapa para inserir os resultados.
     */
    void calculate_cb6_silhouette_coefficient(const arma::mat& data,
                                              const arma::Row<size_t>& assignments,
                                              size_t num_clusters,
                                              const std::string& eps_suffix,
                                              FeatureSet& features) const;

    /**
     * @brief Calcula e salva as features de alcance do cluster (CB4).
     * @param data A matriz de dados dos nós.
     * @param assignments O vetor de atribuição de cluster para cada ponto.
     * @param num_clusters O número total de clusters encontrados.
     * @param eps_suffix O sufixo da feature.
     * @param features O mapa para inserir os resultados.
     */
    void calculate_cb4_cluster_reach(const arma::mat& data,
                                     const arma::Row<size_t>& assignments,
                                     size_t num_clusters,
                                     const std::string& eps_suffix,
                                     FeatureSet& features) const;
};

#endif /* CLUSTERINGFEATURECALCULATOR_H_ */