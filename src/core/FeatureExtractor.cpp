
#include <cvrp-feature-extractor/core/FeatureExtractor.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>
#include <cvrp-feature-extractor/extractors/DirectStatisticsCalculator.h>
#include <cvrp-feature-extractor/extractors/GeometricFeatureCalculator.h>
#include <cvrp-feature-extractor/extractors/GraphFeatureCalculator.h>
#include <cvrp-feature-extractor/extractors/VRPSpecificFeatureCalculator.h>
#include <cvrp-feature-extractor/extractors/ClusteringFeatureCalculator.h>
#include <cvrp-feature-extractor/extractors/ProbingFeatureCalculator.h>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <set>
#include <numeric>
#include <algorithm>
#include <limits>

ExtractionResult FeatureExtractor::extractFeatures(const CVRP& problem) const {
    ExtractionResult result;
    result.context.n = problem.getDimension();
    
    DirectStatisticsCalculator ds_calculator;
    ds_calculator.calculate(problem, result.features);

    GeometricFeatureCalculator gc_calculator;
    GeometricContext geo_context = gc_calculator.calculate(problem, result.features);
    result.context.bb_width = geo_context.bb_width;
    result.context.bb_height = geo_context.bb_height;

     // --- Captura de dados de contexto para uso futuro ---
    auto it_w = result.features.find("GC3.width_bb"); // Use a chave correta
    if (it_w != result.features.end()) {
        result.context.bb_width = it_w->second;
    }
    auto it_h = result.features.find("GC3.height_bb"); // Use a chave correta
    if (it_h != result.features.end()) {
        result.context.bb_height = it_h->second;
    }

    result.context.k_values["GB4.2_k3"] = 3;
    result.context.k_values["GB4.2_k5"] = 5;
    result.context.k_values["GB4.2_k7"] = 7;

    GraphFeatureCalculator gb_calc;
    gb_calc.calculate(problem, result.features);

    // 1. A calculadora de Clustering é chamada ANTES da VRP-specific.
    ClusteringFeatureCalculator cb_calc;
    ClusteringResult clustering_result = cb_calc.calculate(problem, result.features);

    // 2. A calculadora VRP-specific AGORA RECEBE o resultado do clustering
    //    para que possa calcular as features híbridas VS7.
    VRPSpecificFeatureCalculator vs_calc;
    vs_calc.calculate(problem, clustering_result, result.features);

    ProbingFeatureCalculator pt_calc;
    pt_calc.calculate(problem, result.features);

    return result;
}