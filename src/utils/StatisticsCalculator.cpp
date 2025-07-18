
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>

StatisticsCalculator::StatsResults StatisticsCalculator::calculateAll(std::vector<double>& data) {
    StatsResults res;
    if (data.empty()) {
        return res; // Retorna struct com zeros se não houver dados.
    }

    std::sort(data.begin(), data.end());

    res.min = data.front();
    res.max = data.back();

    // Média
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    res.mean = sum / data.size();

    // Mediana e Quartis
    size_t n = data.size();
    res.median = (n % 2 != 0) ? data[n / 2] : (data[n / 2 - 1] + data[n / 2]) / 2.0;
    res.q1 = data[n / 4];
    res.q3 = data[n * 3 / 4];
    
    double sum_sq_diff = 0.0;
    for (const auto& val : data) {
        sum_sq_diff += (val - res.mean) * (val - res.mean);
    }

    // Desvio Padrão
    res.std_dev = std::sqrt(sum_sq_diff / data.size());

    // CV, Skewness, Kurtosis
    if (res.mean != 0) res.cv = res.std_dev / res.mean;

    if (res.std_dev != 0) {
        double sum_cub_diff = 0.0;
        double sum_qua_diff = 0.0;
        for (const auto& val : data) {
            double term = (val - res.mean) / res.std_dev;
            sum_cub_diff += std::pow(term, 3);
            sum_qua_diff += std::pow(term, 4);
        }
        res.skewness = sum_cub_diff / data.size();
        res.kurtosis = sum_qua_diff / data.size();
    }

    return res;
}