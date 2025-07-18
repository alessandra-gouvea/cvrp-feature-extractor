
#include <cvrp-feature-extractor/core/FeatureExtractor.h>
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>
#include <cvrp-feature-extractor/extractors/DirectStatisticsCalculator.h>
#include <cvrp-feature-extractor/extractors/GeometricFeatureCalculator.h>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <set>
#include <numeric>
#include <algorithm>
#include <limits>

FeatureSet FeatureExtractor::extractFeatures(const CVRP& problem) const {
    FeatureSet features;
    
    DirectStatisticsCalculator ds_calculator;
    ds_calculator.calculate(problem, features);

    GeometricFeatureCalculator gc_calculator;
    gc_calculator.calculate(problem, features);

    // 3. Futuramente outros especialistas

    return features;
}