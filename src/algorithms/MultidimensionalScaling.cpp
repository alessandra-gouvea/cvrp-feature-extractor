// Em "MultidimensionalScaling.cpp"

#include <cvrp-feature-extractor/algorithms/MultidimensionalScaling.h>
#include <stdexcept>

// --- Dependência da Biblioteca Eigen ---
// A implementação do MDS depende de uma biblioteca de álgebra linear.
// Eigen é uma escolha padrão e de alta performance em C++.
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

std::vector<std::pair<double, double>> MultidimensionalScaling::calculateCoordinates(
    const std::vector<double>& lower_row_distances, 
    int dimension) const 
{
    // --- PASSO 0: Validação da Entrada ---
    // O número de elementos em uma matriz triangular inferior (sem a diagonal) é N * (N - 1) / 2
    if (dimension <= 1) {
        throw std::invalid_argument("MDS Error: Dimension must be greater than 1.");
    }
    size_t expected_size = static_cast<size_t>(dimension) * (dimension - 1) / 2;
    if (lower_row_distances.size() != expected_size) {
        throw std::invalid_argument("MDS Error: Size of distance vector is inconsistent with the provided dimension.");
    }
    
    // --- PASSO 1: Reconstruir a Matriz de Distância Completa (D) ---
    Eigen::MatrixXd dist_matrix(dimension, dimension);
    int k = 0; // Índice para o vetor de distâncias
    for (int i = 0; i < dimension; ++i) {
        for (int j = 0; j < i; ++j) { // Apenas itera na parte triangular inferior
            dist_matrix(i, j) = lower_row_distances[k];
            dist_matrix(j, i) = lower_row_distances[k]; // A matriz é simétrica
            k++;
        }
        dist_matrix(i, i) = 0.0;
    }

    // --- PASSO 2: Criar a Matriz de Distâncias ao Quadrado e Aplicar Duplo Centramento ---
    Eigen::MatrixXd A = dist_matrix.array().square();
    Eigen::MatrixXd centering_matrix = Eigen::MatrixXd::Identity(dimension, dimension) - (1.0 / dimension) * Eigen::MatrixXd::Ones(dimension, dimension);
    Eigen::MatrixXd B = -0.5 * centering_matrix * A * centering_matrix;

    // --- PASSO 3: Decomposição em Autovalores e Autovetores ---
    // Usamos SelfAdjointEigenSolver porque B é simétrica, o que é mais rápido e estável.
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigen_solver(B);
    if (eigen_solver.info() != Eigen::Success) {
        throw std::runtime_error("MDS Error: Failed to compute eigenvalues.");
    }

    Eigen::VectorXd eigenvalues = eigen_solver.eigenvalues();
    Eigen::MatrixXd eigenvectors = eigen_solver.eigenvectors();

    // --- PASSO 4: Construir a Matriz de Coordenadas ---
    // Pegamos os 2 maiores autovalores e seus autovetores correspondentes.
    // O solver do Eigen ordena os autovalores em ordem crescente.
    double lambda1 = eigenvalues(dimension - 1);
    double lambda2 = eigenvalues(dimension - 2);

    // Clamping: Se os autovalores forem negativos (distâncias não-Euclidianas), trate-os como zero.
    double sqrt_lambda1 = (lambda1 > 0) ? std::sqrt(lambda1) : 0.0;
    double sqrt_lambda2 = (lambda2 > 0) ? std::sqrt(lambda2) : 0.0;
    
    Eigen::VectorXd v1 = eigenvectors.col(dimension - 1);
    Eigen::VectorXd v2 = eigenvectors.col(dimension - 2);
    
    Eigen::MatrixXd coords_matrix(dimension, 2);
    coords_matrix.col(0) = v1 * sqrt_lambda1;
    coords_matrix.col(1) = v2 * sqrt_lambda2;

    // --- PASSO 5: Converter a Matriz Eigen para o Formato de Retorno Padrão ---
    std::vector<std::pair<double, double>> result_coords;
    result_coords.reserve(dimension); // Pre-aloca memória para eficiência
    for (int i = 0; i < dimension; ++i) {
        result_coords.emplace_back(coords_matrix(i, 0), coords_matrix(i, 1));
    }

    return result_coords;
}