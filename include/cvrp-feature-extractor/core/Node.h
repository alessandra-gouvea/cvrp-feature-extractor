/*
 * Node.h
 *
 *  Created on: 26 de abr de 2024
 *      Author: Alessandra M M M Gouvea
 */

#ifndef NODE_H_
#define NODE_H_

#include <cmath> // Para std::sqrt

/**
 * Esta classe encapsula as propriedades de um nó, incluindo sua identidade,
 * coordenadas espaciais, e demanda.
 */
class Node {
public:
    explicit Node(int id, double x, double y);

	int getId() const;
    double getX() const;
    double getY() const;
    int getDemand() const;
    bool isDepot() const;

    double calculateDistanceTo(const Node& other) const;
    void setDemand(int new_demand);
    void setAsDepot();


private:
    const int id_; // O ID de um nó não deve mudar após a criação.
    const double x_;
    const double y_;

    int demand_;
    bool is_depot_;
};

#endif /* NODE_H_ */