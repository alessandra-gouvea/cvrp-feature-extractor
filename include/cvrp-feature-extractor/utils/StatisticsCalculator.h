#ifndef STATISTICSCALCULATOR_H_
#define STATISTICSCALCULATOR_H_

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>
#include <numeric>

class StatisticsCalculator {
public:
    // Struct para guardar todos os resultados de uma vez.
    struct StatsResults {
        double mean = 0.0;
        double std_dev = 0.0;
        double skewness = 0.0;
        double kurtosis = 0.0;
        double cv = 0.0;
        double min = 0.0;
        double max = 0.0;
        double median = 0.0;
        double q1 = 0.0;
        double q3 = 0.0;
    };

    /**
     * @brief Calcula um conjunto completo de descritores estatísticos para um vetor de dados.
     * @param data O multiconjunto de valores numéricos.
     * @return Uma struct StatsResults com todos os valores calculados.
     */
    static StatsResults calculateAll(std::vector<double>& data);
};

#endif /* STATISTICSCALCULATOR_H_ */