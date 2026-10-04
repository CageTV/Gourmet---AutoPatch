cmake_minimum_required(VERSION 3.21)

# GourmetAutoPatch.dll - an SKSE add-on for Gourmet - A Cooking Overhaul: makes other mods' foods follow Gourmet's rules at game start, so they
# need no patch plugin, and shows what it did in SKSE Menu Framework.
project(GourmetAutoPatch VERSION 1.0.0 LANGUAGES CXX)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} NAME "GourmetAutoPatch" AUTHOR "CageTV" SOURCES
    src/main.cpp
    src/Config.cpp
    src/Classifier.cpp
    src/Conform.cpp
    src/Menu.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE src include)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Z7>)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
