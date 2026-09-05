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

    // Estrutura para os resultados da análise de moda
    struct ModalStatsResults {
        int num_modes = 0;
        double primary_mode_value = 0.0;
        double primary_mode_freq_norm = 0.0;
        double mean_of_modal_values = 0.0;
    };

    /**
     * @brief Calcula um conjunto completo de descritores estatísticos para um vetor de dados.
     * @param data O multiconjunto de valores numéricos.
     * @return Uma struct StatsResults com todos os valores calculados.
     */
    static StatsResults calculateAll(std::vector<double>& data);
    static ModalStatsResults analyzeModes(const std::vector<double>& data, double bin_width = 1.0);

     /**
     * @brief Analisa os modos em um conjunto de dados circulares (ângulos em radianos).
     * @param data Vetor de ângulos em radianos, no intervalo [-PI, PI].
     * @param num_points O número de pontos para amostrar a função de densidade.
     * @param bandwidth A "largura" do kernel gaussiano.
     * @return O número de modos (picos) encontrados.
     */
    static int analyzeCircularModes(const std::vector<double>& data, int num_points = 360, double bandwidth = 0.1);
};

#endif /* STATISTICSCALCULATOR_H_ */