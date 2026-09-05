#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <memory> // Para std::unique_ptr

// --- NOSSAS CLASSES ---
#include <cvrp-feature-extractor/io/CVRParser.h>
#include <cvrp-feature-extractor/io/CSVExporter.h>
#include <cvrp-feature-extractor/core/CVRP.h>
#include <cvrp-feature-extractor/core/Node.h>
#include <cvrp-feature-extractor/core/FeatureExtractor.h>
#include <cvrp-feature-extractor/core/NormalizationEngine.h>

std::vector<std::string> findFilesByExtension(const std::string& directory_path, const std::string& extension) {
    std::vector<std::string> file_paths;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
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
    // --- 1. Define os caminhos e a opção de normalização ---
    //const std::string instances_path = "../data/instances";
    //const std::string instances_path = "../data/instances_xxl";
    //const std::string raw_output_path = "../data/output/features_raw_instances_xxl.csv";
    //const std::string norm_output_path = "../data/output/features_normalized_instances_xxl.csv";

    const std::string instances_path = "../data/instances_XL_3";
    const std::string raw_output_path = "../data/output/features_raw_instances_XL_3.csv";
    const std::string norm_output_path = "../data/output/features_normalized_instances_XL_3.csv";
    
    const bool calculate_normalized_features = true; // Controla se o arquivo normalizado é gerado

    // --- 2. Encontra automaticamente todos os arquivos .vrp ---
    std::vector<std::string> instance_files = findFilesByExtension(instances_path, ".vrp");
    if (instance_files.empty()) {
        std::cerr << "Nenhum arquivo .vrp foi encontrado. Encerrando o programa." << std::endl;
        return 1;
    }
    std::cout << "Encontrado(s) " << instance_files.size() << " arquivo(s) de instância para processar." << std::endl;

    try {
        // --- 3. Cria os exportadores de CSV ---
        CSVExporter raw_exporter(raw_output_path);
        std::cout << "Arquivo de saída (features brutas): '" << raw_output_path << "'." << std::endl;

        std::unique_ptr<CSVExporter> norm_exporter = nullptr;
        if (calculate_normalized_features) {
            norm_exporter = std::make_unique<CSVExporter>(norm_output_path);
            std::cout << "Arquivo de saída (features normalizadas): '" << norm_output_path << "'." << std::endl;
        }

        // --- Cria os objetos reutilizáveis ---
        FeatureExtractor extractor;
        NormalizationEngine normalizer;

        // --- 4. Itera sobre cada arquivo de instância encontrado ---
        for (const auto& filename : instance_files) {
            std::cout << "\n==================================================" << std::endl;
            std::cout << "Processando: '" << filename << "'" << std::endl;
            
            try {
                // ETAPAS A, B, C: Parsear, construir e extrair
                CVRParser parser;
                ProblemData data = parser.loadProblemFile(filename);
                CVRP problem(data);
                ExtractionResult extraction_result = extractor.extractFeatures(problem);
                
                // ETAPA D.1: Sempre escreve as features brutas
                raw_exporter.writeRow(problem.getName(), extraction_result.features);
                std::cout << "--> Features brutas para '" << problem.getName() << "' salvas." << std::endl;
                
                // ETAPA D.2 (Opcional): LÓGICA FINAL para o conjunto normalizado
                if (calculate_normalized_features && norm_exporter) {
                    
                    // a) Calcula um mapa contendo APENAS os novos valores normalizados, com as CHAVES ORIGINAIS.
                    FeatureSet normalized_values = normalizer.normalize(
                        extraction_result.features, 
                        extraction_result.context
                    );

                    // b) Começa com uma CÓPIA de TODAS as features originais.
                    FeatureSet fully_normalized_set = extraction_result.features;
                    
                    // c) ATUALIZA o conjunto com os novos valores.
                    // Este loop sobrescreve os valores das chaves que existem em 'normalized_values'.
                    for (const auto& pair : normalized_values) {
                        fully_normalized_set[pair.first] = pair.second;
                    }

                    // e) Escreve o conjunto completo e atualizado no arquivo normalizado
                    norm_exporter->writeRow(problem.getName(), fully_normalized_set);
                    std::cout << "--> Features normalizadas para '" << problem.getName() << "' salvas." << std::endl;
                }

            } catch (const std::exception& e) {
                std::cerr << "--> ERRO ao processar '" << filename << "': " << e.what() << std::endl;
                std::cerr << "--> Pulando para a próxima instância." << std::endl;
            }
        }

        std::cout << "\n==================================================" << std::endl;
        std::cout << "Processamento em lote concluído com sucesso." << std::endl;
        std::cout << "Resultados salvos em '" << raw_output_path << "'";
        if (calculate_normalized_features) {
            std::cout << " e '" << norm_output_path << "'";
        }
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Erro fatal que impediu a execução: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}