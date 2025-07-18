/*
 * Node.cpp
 *
 *  Created on: 26 de abr de 2024
 *      Author: Alessandra M M M Gouvea
 */

#include <cvrp-feature-extractor/core/Node.h>

Node::Node(int id, double x, double y)
    : id_(id), 
      x_(x), 
      y_(y), 
      demand_(0),      // Por padrão, a demanda é 0.
      is_depot_(false) // Por padrão, um nó não é um depósito.
{

}

int Node::getId() const {
    return id_;
}

double Node::getX() const {
    return x_;
}

double Node::getY() const {
    return y_;
}

int Node::getDemand() const {
    return demand_;
}

bool Node::isDepot() const {
    return is_depot_;
}

void Node::setDemand(int new_demand) {
    // Um nó não pode ter demanda negativa.
    if (!is_depot_ && new_demand >= 0) {
        this->demand_ = new_demand;
    }
}

void Node::setAsDepot() {
    this->is_depot_ = true;
    this->demand_ = 0; // A demanda de um depósito é, por definição, zero.
}

double Node::calculateDistanceTo(const Node& other) const {
    const double dx = this->x_ - other.x_;
    const double dy = this->y_ - other.y_;
    return std::sqrt(dx * dx + dy * dy);
}