# Setup cxxopts as a local library to reduce external dependency requirements.

add_library(cxxopts INTERFACE include/cxxopts.hpp)

target_include_directories(cxxopts INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
