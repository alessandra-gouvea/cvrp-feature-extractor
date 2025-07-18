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

// O método público principal que orquestra a escrita.
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