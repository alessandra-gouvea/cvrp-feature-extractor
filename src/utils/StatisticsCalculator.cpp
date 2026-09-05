
#include <cvrp-feature-extractor/utils/StatisticsCalculator.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

StatisticsCalculator::ModalStatsResults StatisticsCalculator::analyzeModes(const std::vector<double>& data, double bin_width) {
    ModalStatsResults res;
    if (data.empty()) return res;

    // Cria um histograma para encontrar os modos
    std::map<int, int> histogram;
    for (double val : data) {
        // Agrupa os valores em 'bins' para lidar com ponto flutuante
        histogram[static_cast<int>(std::round(val / bin_width))]++;
    }

    if (histogram.empty()) return res;

    // Encontra a frequência máxima
    int max_freq = 0;
    for (const auto& pair : histogram) {
        if (pair.second > max_freq) {
            max_freq = pair.second;
        }
    }

    if (max_freq <= 1) { // Considera que não há modo se tudo for único
        res.num_modes = 0;
        res.primary_mode_value = data[0]; // Retorna um valor padrão
        res.primary_mode_freq_norm = 1.0 / data.size();
        res.mean_of_modal_values = data[0];
        return res;
    }

    // Encontra todos os modos com a frequência máxima
    std::vector<double> modes;
    for (const auto& pair : histogram) {
        if (pair.second == max_freq) {
            modes.push_back(static_cast<double>(pair.first) * bin_width);
        }
    }
    
    res.num_modes = modes.size();
    if (!modes.empty()) {
        // A moda primária pode ser a primeira ou a média de todas
        res.primary_mode_value = modes[0]; 
        res.mean_of_modal_values = std::accumulate(modes.begin(), modes.end(), 0.0) / modes.size();
    }
    res.primary_mode_freq_norm = static_cast<double>(max_freq) / data.size();
    
    return res;
}

int StatisticsCalculator::analyzeCircularModes(const std::vector<double>& data, int num_points, double bandwidth) {
    if (data.size() < 3) return 1; // Não é possível encontrar múltiplos modos com poucos pontos.

    std::vector<double> density(num_points, 0.0);
    
    // Constrói a função de densidade de kernel
    for (int i = 0; i < num_points; ++i) {
        double eval_point = -M_PI + (2.0 * M_PI * i) / num_points;
        double total_density = 0.0;
        for (double angle : data) {
            // Diferença angular, lidando com o "wrap-around"
            double diff = angle - eval_point;
            while (diff <= -M_PI) diff += 2.0 * M_PI;
            while (diff > M_PI) diff -= 2.0 * M_PI;
            
            // Kernel Gaussiano
            total_density += std::exp(-0.5 * std::pow(diff / bandwidth, 2));
        }
        density[i] = total_density;
    }

    // Conta os picos (máximos locais) na função de densidade
    int num_modes = 0;
    for (int i = 0; i < num_points; ++i) {
        // Pega os vizinhos, lidando com o "wrap-around" do círculo
        double prev_val = density[(i - 1 + num_points) % num_points];
        double current_val = density[i];
        double next_val = density[(i + 1) % num_points];
        
        if (current_val > prev_val && current_val > next_val) {
            num_modes++;
        }
    }

    // Se nenhum pico for encontrado (ex: dados uniformes), retorne 1 modo.
    return (num_modes == 0) ? 1 : num_modes;
}