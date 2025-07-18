/**
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  Ponto de entrada principal para a ferramenta de extração de
 *                  features de instâncias CVRP. Este programa irá:
 *                  1. Procurar automaticamente por todos os arquivos .vrp no diretório 'data/instances'.
 *                  2. Para cada arquivo encontrado, carregar, parsear e construir um objeto CVRP.
 *                  3. Extrair um conjunto de features de caracterização do objeto CVRP.
 *                  4. Escrever os resultados, uma linha por instância, em um arquivo CSV.
 *
 *        Version:  1.0
 *        Created:  25/07/2024
 *       Compiler:  g++ (ou Clang, MSVC) com suporte a C++17
 *
 * =====================================================================================
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

// --- NOSSAS CLASSES ---
#include <cvrp-feature-extractor/io/CVRParser.h>
#include <cvrp-feature-extractor/io/CSVExporter.h>
#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/Node.h>
#include <cvrp-feature-extractor/core/FeatureExtractor.h>

/**
 * @brief Procura por todos os arquivos com uma extensão específica em um diretório.
 *
 * @param directory_path O caminho para o diretório a ser pesquisado.
 * @param extension A extensão do arquivo a ser procurado (ex: ".vrp").
 * @return Um vetor de strings contendo os caminhos completos para os arquivos encontrados.
 */
std::vector<std::string> findFilesByExtension(const std::string& directory_path, const std::string& extension) {
    std::vector<std::string> file_paths;
    try {
        // Itera sobre cada entrada no diretório especificado
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            // Verifica se a entrada é um arquivo regular e se a extensão corresponde
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Erro ao acessar o diretório '" << directory_path << "': " << e.what() << std::endl;
        std::cerr << "Por favor, verifique se o diretório existe." << std::endl;
    }
    return file_paths;
}

int main() {
    // --- 1. Define os caminhos de entrada e saída ---
    const std::string instances_path = "../data/instances";
    const std::string output_csv_path = "../data/output/feature_results.csv";

    // --- 2. Encontra automaticamente todos os arquivos .vrp ---
    std::cout << "Procurando por arquivos .vrp em '" << instances_path << "'..." << std::endl;
    std::vector<std::string> instance_files = findFilesByExtension(instances_path, ".vrp");

    if (instance_files.empty()) {
        std::cerr << "Nenhum arquivo .vrp foi encontrado. Encerrando o programa." << std::endl;
        return 1;
    }
    std::cout << "Encontrado(s) " << instance_files.size() << " arquivo(s) de instância para processar." << std::endl;

    try {
        // --- 3. Cria o exportador CSV UMA VEZ, antes do loop de processamento ---
        CSVExporter exporter(output_csv_path);
        std::cout << "Arquivo de saída '" << output_csv_path << "' aberto para escrita." << std::endl;

        // --- 4. Itera sobre cada arquivo de instância encontrado ---
        for (const auto& filename : instance_files) {
            std::cout << "\n==================================================" << std::endl;
            std::cout << "Processando: '" << filename << "'" << std::endl;
            
            try {
                // ETAPA A: Parsear o arquivo para um objeto de dados bruto
                CVRParser parser;
                ProblemData data = parser.loadProblemFile(filename);

                // ETAPA B: Construir o modelo de domínio CVRP a partir dos dados brutos
                CVRP problem(data);

                // ETAPA C: Extrair as features do modelo de domínio
                FeatureExtractor extractor;
                std::map<std::string, double> features = extractor.extractFeatures(problem);
                
                // ETAPA D: Escrever a linha de resultado no arquivo CSV
                exporter.writeRow(problem.getName(), features);
                
                std::cout << "--> Sucesso! Features para '" << problem.getName() << "' salvas no CSV." << std::endl;

            } catch (const std::exception& e) {
                // Tratamento de erro para UMA instância. O programa não para.
                std::cerr << "--> ERRO ao processar '" << filename << "': " << e.what() << std::endl;
                std::cerr << "--> Pulando para a próxima instância." << std::endl;
            }
        }

        std::cout << "\n==================================================" << std::endl;
        std::cout << "Processamento em lote concluído com sucesso." << std::endl;
        std::cout << "Resultados salvos em '" << output_csv_path << "'" << std::endl;

    } catch (const std::exception& e) {
        // Erro fatal (ex: não conseguiu criar o arquivo CSV). O programa para.
        std::cerr << "Erro fatal que impediu a execução: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}