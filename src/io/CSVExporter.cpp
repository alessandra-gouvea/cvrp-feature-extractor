#include <cvrp-feature-extractor/io/CSVExporter.h>
#include <cvrp-feature-extractor/core/FeatureExtractor.h>
#include <stdexcept>
#include <iostream>

CSVExporter::CSVExporter(const std::string& output_filename) {
    m_outputFile.open(output_filename);
    if (!m_outputFile.is_open()) {
        throw std::runtime_error("CSVExporter Error: Could not open file for writing: " + output_filename);
    }
}

// O método privado que lida apenas com o cabeçalho.
void CSVExporter::writeHeader(const FeatureSet& features) {
    // Escreve a primeira coluna para o nome da instância.
    m_outputFile << "InstanceName";

    // Itera sobre todas as features e escreve suas CHAVES (nomes).
    for (const auto& pair : features) {
        m_outputFile << "," << pair.first;
    }

    // Termina a linha do cabeçalho.
    m_outputFile << "\n";
}

// O método privado que agora usa a lista de cabeçalhos da classe.
void CSVExporter::writeHeader() {
    // Escreve a primeira coluna para o nome da instância.
    m_outputFile << "InstanceName";

    // Itera sobre a variável membro m_headers, que contém a ordem
    // correta e fixa das colunas.
    for (const auto& header : m_headers) {
        m_outputFile << "," << header;
    }

    // Termina a linha do cabeçalho.
    m_outputFile << "\n";
}

// O método público principal que orquestra a escrita.
/*
void CSVExporter::writeRow(const std::string& instance_name, const FeatureSet& features) {
    // Se o cabeçalho ainda não foi escrito, escreva-o agora.
    if (!m_headerWritten) {
        writeHeader(features);
        m_headerWritten = true;
    }

    // Escreve o nome da instância, seguido por uma vírgula.
    m_outputFile << instance_name;

    // Itera sobre todas as features e escreve seus VALORES.
    // Como std::map é ordenado, a ordem dos valores corresponderá à ordem do cabeçalho.
    for (const auto& pair : features) {
        m_outputFile << "," << pair.second;
    }

    // Termina a linha.
    m_outputFile << "\n";
}
*/

// O método público principal que orquestra a escrita.
void CSVExporter::writeRow(const std::string& instance_name, const FeatureSet& features) {
    // Se o cabeçalho ainda não foi escrito (ou seja, se esta é a primeira linha de dados),
    // usamos as chaves deste mapa para DEFINIR a nossa lista mestra de cabeçalhos.
    if (!m_headerWritten) {
        // Popula nossa variável membro m_headers com todas as chaves do mapa,
        // que já estarão em ordem alfabética por ser um std::map.
        m_headers.reserve(features.size());
        for (const auto& pair : features) {
            m_headers.push_back(pair.first);
        }
        
        // Agora que m_headers está definida, chama a nova writeHeader() para escrever no arquivo.
        writeHeader(); // A chamada agora está correta, sem argumentos.
        m_headerWritten = true;
    }

    // Escreve o nome da instância, a primeira coluna da linha.
    m_outputFile << instance_name;

    // --- LÓGICA DE ESCRITA ROBUSTA ---
    // Em vez de iterar sobre o mapa 'features', nós iteramos sobre a nossa
    // lista mestra 'm_headers', que tem a ordem fixa e correta.
    for (const auto& header : m_headers) {
        m_outputFile << ",";
        
        // Para cada cabeçalho, procuramos a chave correspondente no mapa de features da instância ATUAL.
        auto it = features.find(header);
        
        if (it != features.end()) {
            // Se a feature foi encontrada para esta instância, escrevemos seu valor.
            m_outputFile << it->second;
        }
        // Se 'it == features.end()', significa que esta instância não calculou esta feature.
        // Neste caso, não escrevemos nada, o que resulta em uma célula vazia no CSV.
        // Isso garante que as colunas seguintes permaneçam perfeitamente alinhadas.
    }

    // Termina a linha de dados com uma quebra de linha.
    m_outputFile << "\n";
}