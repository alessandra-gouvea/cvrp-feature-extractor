# Versão mínima do CMake necessária.
cmake_minimum_required(VERSION 3.16)

project(cvrp-feature-extractor LANGUAGES CXX)

include(ExternalProject)

# --- CONFIGURAÇÃO PARA HOMEBREW ---
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew)
endif()

# --- DEPENDÊNCIAS EXTERNAS ---
find_package(Boost 1.70 REQUIRED CONFIG COMPONENTS system thread graph)
find_package(Eigen3 3.3 REQUIRED)
find_package(Armadillo REQUIRED)
find_package(ensmallen REQUIRED)
find_package(Cereal REQUIRED)
find_package(ortools REQUIRED)

# --- DEPENDÊNCIA LOCAL (mlpack) ---
# Esta etapa apenas "instala" os headers em um local conhecido para nós.
set(MLPACK_INSTALL_DIR ${CMAKE_BINARY_DIR}/mlpack-install)
ExternalProject_Add(
    mlpack_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/mlpack-4.6.2
    INSTALL_DIR ${MLPACK_INSTALL_DIR}
    # Nós não precisamos de um passo de build/binary_dir se estamos apenas instalando headers.
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${MLPACK_INSTALL_DIR}
    # Força a etapa de instalação a ser executada.
    BUILD_COMMAND ""
    UPDATE_COMMAND ""
)
set(MLPACK_INCLUDE_DIRS ${MLPACK_INSTALL_DIR}/include)

# --- ORGANIZAÇÃO DOS ARQUIVOS FONTE ---
set(CVRP_LIB_SOURCES
    src/core/CVRP.cpp
    src/core/Node.cpp
    src/core/FeatureExtractor.cpp
    src/core/NormalizationEngine.cpp
    src/extractors/DirectStatisticsCalculator.cpp
    src/extractors/GeometricFeatureCalculator.cpp
    src/extractors/GraphFeatureCalculator.cpp
    src/extractors/VRPSpecificFeatureCalculator.cpp
    src/extractors/ClusteringFeatureCalculator.cpp
    src/extractors/ProbingFeatureCalculator.cpp
    src/io/CVRParser.cpp
    src/io/CSVExporter.cpp
    src/algorithms/MultidimensionalScaling.cpp
    src/utils/StatisticsCalculator.cpp
)
set(CVRP_APP_SOURCES src/main.cpp)

# --- DEFINIÇÃO DOS NOSSOS ALVOS ---
add_library(cvrp_lib STATIC ${CVRP_LIB_SOURCES})
add_executable(cvrp_app ${CVRP_APP_SOURCES})

# --- CONFIGURAÇÃO DAS PROPRIEDADES DOS NOSSOS ALVOS ---
target_compile_features(cvrp_lib PUBLIC cxx_std_17)

target_include_directories(cvrp_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${Boost_INCLUDE_DIRS}
        ${ARMADILLO_INCLUDE_DIR}
        ${MLPACK_INCLUDE_DIRS}
    PRIVATE
        ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(cvrp_lib
    PUBLIC
        Boost::system
        Boost::thread
        Boost::graph
        ${ARMADILLO_LIBRARIES}
        ortools::ortools
)

add_dependencies(cvrp_lib mlpack_external)
target_link_libraries(cvrp_app PRIVATE cvrp_lib)