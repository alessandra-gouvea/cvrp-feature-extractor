#ifndef CVRPARSER_H_
#define CVRPARSER_H_

#include <cvrp-feature-extractor/io/ProblemData.h>
#include <string>
#include <utility>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

/*
 * Esta classe é responsável exclusivamente por ler um arquivo de problema do disco
 * e traduzir seu conteúdo para um objeto de transferência de dados (DTO) chamado ProblemData.
 * Ela não possui conhecimento sobre a lógica de negócio do CVRP ou de qualquer solver.
 */
class CVRParser {
public:
    CVRParser() = default;
    ProblemData loadProblemFile(const std::string& filename) const;

private:
    std::pair<std::string, std::string> readKeyValue(const std::string& line) const;
};

#endif /* CVRPARSER_H_ */