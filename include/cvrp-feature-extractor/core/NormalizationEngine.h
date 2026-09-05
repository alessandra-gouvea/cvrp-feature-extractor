#ifndef NORMALIZATIONENGINE_H_
#define NORMALIZATIONENGINE_H_

#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <cvrp-feature-extractor/core/FeatureExtractor.h>
#include <vector>
#include <string>

/**
 * @class NormalizationEngine
 * @brief Aplica a normalização teórica de Heins et al. (2022) a um conjunto
 *        de features pré-calculadas.
 */
class NormalizationEngine {
public:
    NormalizationEngine() = default;

    /**
     * @brief Calcula os valores normalizados para as features aplicáveis.
     * @param original_features O mapa de features extraídas.
     * @param context Os dados brutos da instância necessários para a normalização.
     * @return Um FeatureSet contendo APENAS os pares chave-valor das features que 
     *         foram normalizadas, usando as chaves originais.
     */
    FeatureSet normalize(const FeatureSet& original_features, const NormalizationContext& context) const;

    /**
     * @brief Retorna a lista de substrings que identificam features já
     *        normalizadas ou independentes de escala.
     * @return Um vetor de strings contendo as substrings da "lista negra".
     */
    static std::vector<std::string> getBlacklistSubstrings();

private:
    double calculate_d_max(int n, double a, double b) const;

    void normalize_scc_size_stats(const std::string& key, double value, double n, double k, FeatureSet& normalized_features) const;
    void normalize_wcc_size_stats(const std::string& key, double value, double n, double k, FeatureSet& normalized_features) const;
};

#endif /* NORMALIZATIONENGINE_H_ */