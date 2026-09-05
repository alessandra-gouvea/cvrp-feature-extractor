#include <cvrp-feature-extractor/core/NormalizationEngine.h>
#include <cmath>
#include <stdexcept>

// =========================================================================
// === IMPLEMENTAÇÃO DOS MÉTODOS ===========================================
// =========================================================================

void NormalizationEngine::normalize_scc_size_stats(const std::string& key, double value, double n, double k, FeatureSet& normalized_features) const {
    const std::string& norm_key = key; // Usa a mesma chave original
    double f_min = 0.0, f_max = 0.0;

    if (key.find("_mean_scc_size") != std::string::npos) {
        f_min = n / (n - k);
        f_max = n;
    } else if (key.find("_median_scc_size") != std::string::npos) {
        f_min = 1.0;
        f_max = n;
    } else if (key.find("_max_scc_size") != std::string::npos) {
        f_min = k + 1.0;
        f_max = n;
    } else if (key.find("_min_scc_size") != std::string::npos) {
        f_min = 1.0;
        f_max = n;
    }
    
    if (f_max > f_min) {
        normalized_features[norm_key] = (value - f_min) / (f_max - f_min);
    }
}

void NormalizationEngine::normalize_wcc_size_stats(const std::string& key, double value, double n, double k, FeatureSet& normalized_features) const {
    const std::string& norm_key = key; // Usa a mesma chave original
    double f_min = 0.0, f_max = 0.0;
    
    if ((n / 3.0) < (k + 1.0)) {
        return;
    }

    double max_num_wcc = std::floor(n / (k + 1.0));

    if (key.find("_mean_wcc_size") != std::string::npos) {
        f_min = n / max_num_wcc;
        f_max = n;
    } else if (key.find("_median_wcc_size") != std::string::npos) {
        f_min = k + 1.0;
        f_max = n;
    } else if (key.find("_max_wcc_size") != std::string::npos) {
        f_min = (k + 1.0) + (fmod(n, k + 1.0)) / max_num_wcc;
        f_max = n;
    } else if (key.find("_min_wcc_size") != std::string::npos) {
        f_min = k + 1.0;
        f_max = n;
    }
    
    if (f_max > f_min) {
        normalized_features[norm_key] = (value - f_min) / (f_max - f_min);
    }
}

double NormalizationEngine::calculate_d_max(int n, double a, double b) const {
    if (n < 2) return 0.0;
    if (a < 0 || b < 0) throw std::invalid_argument("Dimensões do Bounding Box devem ser não-negativas.");
    if (a == 0 && b == 0) return 0.0;

    const double n_minus_1 = static_cast<double>(n - 1);
    const double a_plus_b = a + b;
    const double a_times_b = a * b;
    const double sqrt3 = std::sqrt(3.0);
    const double discriminant = (a_plus_b * a_plus_b) + (8.0 * n_minus_1 * a_times_b) / sqrt3;
    const double numerator = a_plus_b + std::sqrt(discriminant);
    const double denominator = 2.0 * n_minus_1;
    
    return numerator / denominator;
}

std::vector<std::string> NormalizationEngine::getBlacklistSubstrings() {
    return {
        "_cv", "_skew", "_kurt", "_frac", "_ratio", "_coeff", "_num_", "indegree",
        "_degree", "_size", "_angle", "_cosine", "IsDepotOutside", "norm_sum_mst_costs" 
    };
}


FeatureSet NormalizationEngine::normalize(const FeatureSet& original_features, const NormalizationContext& context) const {
    FeatureSet normalized_features; // Retorna APENAS os valores que foram alterados
    if (context.n < 2) {
        return normalized_features;
    }

    const double d_max = calculate_d_max(context.n, context.bb_width, context.bb_height);
    const double d_max_sq = (d_max > 0) ? d_max * d_max : 0;
    const double n = static_cast<double>(context.n);

    if (d_max <= 1e-9) {
        return normalized_features;
    }
    
    const std::vector<std::string> blacklist_substrings = getBlacklistSubstrings();

    for (const auto& pair : original_features) {
        const std::string& key = pair.first;
        const double value = pair.second;
        const std::string& norm_key = key; // <<<<<< IMPORTANTE: SEMPRE A MESMA CHAVE

        bool is_blacklisted = false;
        if (key.find("num_scc") == std::string::npos && 
            key.find("num_wcc") == std::string::npos && 
            key.find("mst_depth_max") == std::string::npos &&
            key.find("scc_size") == std::string::npos &&
            key.find("wcc_size") == std::string::npos) 
        {
            for (const auto& blacklisted : blacklist_substrings) {
                if (key.find(blacklisted) != std::string::npos) {
                    is_blacklisted = true;
                    break;
                }
            }
        }
        
        if (is_blacklisted) {
            continue;
        }

        if (key.find("scc_size") != std::string::npos) {
            for (const auto& k_pair : context.k_values) {
                if (key.find(k_pair.first) != std::string::npos) {
                    normalize_scc_size_stats(key, value, n, static_cast<double>(k_pair.second), normalized_features);
                    break;
                }
            }
        }
        else if (key.find("wcc_size") != std::string::npos) {
            for (const auto& k_pair : context.k_values) {
                if (key.find(k_pair.first) != std::string::npos) {
                    normalize_wcc_size_stats(key, value, n, static_cast<double>(k_pair.second), normalized_features);
                    break;
                }
            }
        }
        else if (key.find("distance") != std::string::npos || key.find("dist") != std::string::npos ||
                 key.find("cost") != std::string::npos || key.find("length") != std::string::npos ||
                 key.find("SLEV") != std::string::npos || key.find("SL25P") != std::string::npos ||
                 key.find("SH25P") != std::string::npos || key.find("ERTL") != std::string::npos ||
                 key.find("mode_value") != std::string::npos ||
                 key.find("reach") != std::string::npos || key.find("radius") != std::string::npos ||
                 key.find("span") != std::string::npos || key.find("depth") != std::string::npos ||
                 key.find("mbc") != std::string::npos)
        {
            normalized_features[norm_key] = value / d_max;
        }
        else if (key.find("_area") != std::string::npos) {
            normalized_features[norm_key] = value / d_max_sq;
        }
        else if (key.find("mst_depth_max") != std::string::npos) {
            double f_max = std::ceil(n / 2.0);
            double f_min = 1.0;
            if (f_max > f_min) {
                normalized_features[norm_key] = (value - f_min) / (f_max - f_min);
            }
        }
        else if (key.find("num_scc") != std::string::npos) {
             for (const auto& k_pair : context.k_values) {
                if (key.find(k_pair.first) != std::string::npos) {
                    double k = static_cast<double>(k_pair.second);
                    double f_max = n - k;
                    double f_min = 1.0;
                    if (f_max > f_min) {
                        normalized_features[norm_key] = (value - f_min) / (f_max - f_min);
                    }
                    break;
                }
            }
        }
        else if (key.find("num_wcc") != std::string::npos) {
            for (const auto& k_pair : context.k_values) {
                if (key.find(k_pair.first) != std::string::npos) {
                    double k = static_cast<double>(k_pair.second);
                    double f_max = std::floor(n / (k + 1.0));
                    double f_min = 1.0;
                    if (f_max > f_min) {
                        normalized_features[norm_key] = (value - f_min) / (f_max - f_min);
                    }
                    break;
                }
            }
        }
    }
    return normalized_features;
}