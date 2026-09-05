#ifndef CSVEXPORTER_H_
#define CSVEXPORTER_H_

#include <cvrp-feature-extractor/core/FeatureTypes.h>
#include <string>
#include <fstream>
#include <map>
#include <vector>


/**
 * @class CSVExporter
 * @brief Responsável por escrever os resultados da extração de features em um arquivo CSV.
 */
class CSVExporter {
public:
    /**
     * @brief Constrói o exportador e abre o arquivo de saída.
     * @param output_filename O caminho para o arquivo CSV a ser criado/sobrescrito.
     * @throw std::runtime_error se o arquivo não puder ser aberto.
     */
    explicit CSVExporter(const std::string& output_filename);

    /**
     * @brief Escreve uma linha de dados no arquivo CSV.
     *        Escreverá o cabeçalho automaticamente na primeira chamada.
     * @param instance_name O nome da instância (primeira coluna).
     * @param features O conjunto de features calculadas para essa instância.
     */
    void writeRow(const std::string& instance_name, const FeatureSet& features);

private:
    std::ofstream m_outputFile;
    bool m_headerWritten = false;
    std::vector<std::string> m_headers;

    /**
     * @brief Método auxiliar para escrever o cabeçalho do CSV.
     * @param features Um conjunto de features de amostra para extrair os nomes das colunas.
     */
    void writeHeader(const FeatureSet& features);
    void writeHeader(); 
};

#endif /* CSVEXPORTER_H_ */