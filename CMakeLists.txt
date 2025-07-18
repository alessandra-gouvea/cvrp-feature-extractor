# Versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(cvrp-feature-extractor)

# Define o padrão do C++ que queremos usar
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Crie a biblioteca a partir dos seus arquivos .cpp.
# Corrigimos os caminhos para apontar para a pasta 'src/'.
add_library(cvrp_lib
    src/core/CVRP.cpp
    src/core/Node.cpp
    src/core/FeatureExtractor.cpp
    src/io/CVRParser.cpp
    src/io/CSVExporter.cpp
    src/algorithms/MultidimensionalScaling.cpp
    src/extractors/DirectStatisticsCalculator.cpp
        src/extractors/GeometricFeatureCalculator.cpp
    src/utils/StatisticsCalculator.cpp
)

# Adiciona um executável chamado "app"
# e lista o arquivo .cpp necessário para construí-lo (o que contém a função main)
add_executable(app
    src/main.cpp
)

# Diz ao compilador onde encontrar os arquivos de cabeçalho (.h) para o alvo "cvrp_lib"
# Isso não muda e está correto.
target_include_directories(cvrp_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/libs/eigen-3.4.0
)

# --- ADIÇÃO CRÍTICA ---
# Diz que o executável "app" precisa da biblioteca "cvrp_lib" para funcionar.
# Isso resolve o problema de "linkagem" que você teria a seguir.
target_link_libraries(app PRIVATE cvrp_lib)