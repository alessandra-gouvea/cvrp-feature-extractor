# CVRP Feature Extractor (C++)

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg) ![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

A C++ tool for the comprehensive extraction of features from Capacitated Vehicle Routing Problem (CVRP) instances. The goal is to convert raw instance files (TSPLib) into feature vectors for use in meta-learning tasks.

## 🎯 Objective

The main objective of this project is to provide a fast and efficient tool for the analysis of CVRP instances, automating the process of extracting features that describe the problem's properties.

## ✨ Key Features

-   **Geometric Feature Extraction:** Metrics related to the dispersion and location of customers.
-   **Statistical Metrics Calculation:** Analysis of demands, distances, and costs.
-   **Structural Analysis:** Features related to the underlying graph structure.

## 🚀 How to Use

### Prerequisites

-   A modern C++ compiler (with support for C++17 or higher), such as `g++` or `Clang`.
-   `CMake` (version 3.10 or higher).
-   `Git`.

🛠️ Technologies Used
* C++17: The core language for performance and control.
* CMake: The build automation system.

#### External Libraries:
*   **Boost (version 1.74 or higher):** A collection of high-quality C++ libraries. It is **required** for advanced geometry and graph calculations.
    *   **Required Modules:** `Boost.Geometry`, `Boost.Graph`.
*   **(Future) mlpack & Armadillo:** For Machine Learning features, such as DBSCAN clustering.

📄 License
This project is licensed under the MIT License. See the LICENSE file for more details.

