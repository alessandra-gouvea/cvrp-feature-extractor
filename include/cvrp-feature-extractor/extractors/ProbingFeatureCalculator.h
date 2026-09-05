#ifndef PROBINGFEATURECALCULATOR_H_
#define PROBINGFEATURECALCULATOR_H_

#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/FeatureTypes.h>

// Forward declarations para tipos do OR-Tools para manter o header limpo
namespace operations_research {
    class RoutingModel;
    class Assignment;
}


/**
 * @class ProbingFeatureCalculator
 * @brief Especialista em calcular features de "sondagem" (PT) usando o Google OR-Tools.
 *
 * Esta classe constrói um modelo de roteamento a partir da instância CVRP,
 * executa o solver do OR-Tools por um tempo limitado e extrai
 * características sobre a qualidade da solução e a dificuldade da busca.
 */
class ProbingFeatureCalculator {
public:
    ProbingFeatureCalculator() = default;

    /**
     * @brief Calcula todas as features PT e as adiciona ao mapa de resultados.
     * @param problem A instância CVRP a ser analisada.
     * @param features O mapa onde as novas features serão inseridas.
     */
    void calculate(const CVRP& problem, FeatureSet& features) const;

private:
    // --- Funções Auxiliares Privadas ---    
    // Analisa a qualidade geral da solução e das rotas individuais
    void analyze_solution_and_routes(
        const CVRP& problem,
        const operations_research::RoutingModel& routing,
        const operations_research::Assignment& solution,
        FeatureSet& features) const;

    // Analisa a geometria das rotas (interseções, formas)
    void analyze_route_geometry(
        const CVRP& problem,
        const operations_research::RoutingModel& routing,
        const operations_research::Assignment& solution,
        FeatureSet& features) const;
};

#endif /* PROBINGFEATURECALCULATOR_H_ */