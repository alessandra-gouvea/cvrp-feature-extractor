
#ifndef FEATURETYPES_H_
#define FEATURETYPES_H_

#include <string>
#include <map>

/**
 * @brief Define o "envelope" que carrega os resultados da extração de features.
 *
 * Usamos um alias para std::map para dar um nome semântico e claro ao nosso
 * conjunto de resultados. A chave é o nome da feature e o valor é o resultado.
 * std::map garante que as features serão ordenadas alfabeticamente,
 * o que é ótimo para uma saída CSV consistente.
 */
using FeatureSet = std::map<std::string, double>;


#endif /* FEATURETYPES_H_ */