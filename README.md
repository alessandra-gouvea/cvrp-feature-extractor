# CVRP Feature Extractor (C++)

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg) ![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

> **⚠️ Status: Under construction**
>
> This code is under active development and should not yet be considered stable.
>
> Use the results with caution until this review is complete.

A C++ tool for the comprehensive extraction of features from Capacitated Vehicle Routing Problem (CVRP) instances. The goal is to convert raw instance files (TSPLib) into feature vectors for use in meta-learning tasks.

## 🎯 Objective

The main objective of this project is to provide a fast and efficient tool for the analysis of CVRP instances, automating the process of extracting features that describe the problem's properties.

## ✨ Key Features

* Geometric Feature Extraction: Metrics related to the dispersion and location of customers.
* Statistical Metrics Calculation: Analysis of demands, distances, and costs.
* Structural Analysis: Features related to the underlying graph structure.
* Clustering Analysis: DBSCAN-based features describing spatial clustering of customers.
* Probing Features: Solver-based features (via OR-Tools) that characterize instance difficulty through partial solving.

## 🚀 How to Use

### Prerequisites

-   A modern C++ compiler (with support for C++17 or higher), such as `g++` or `Clang`.
-   `CMake` (version 3.10 or higher).
-   `Git`.
-   mlpack 4.6.2 sources manually placed in `libs/mlpack-4.6.2` (not included in this repository; the build currently fails without this step).

🛠️ Technologies Used

* C++17: The core language for performance and control.
* CMake: The build automation system.

#### External Libraries:

* Boost (version 1.70 or higher): A collection of high-quality C++ libraries.
   * Required Modules: `system`, `thread`, `graph` (linked). `Boost.Geometry` is used as a header-only library.
* Eigen3 (version 3.3 or higher): Linear algebra library, required by the build.
* Armadillo: Linear algebra backend required by mlpack.
* mlpack (4.6.2): Used for DBSCAN clustering in the clustering feature calculator.
* ensmallen & Cereal: Transitive dependencies required by mlpack.
* OR-Tools: Used for solver-based probing features (e.g., route centroid calculations).

📄 License
This project is licensed under the MIT License. See the LICENSE file for more details.

