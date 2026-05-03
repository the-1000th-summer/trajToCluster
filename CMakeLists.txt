cmake_minimum_required(VERSION 3.20)

project(trajToCluster LANGUAGES CXX)

find_package(netCDFCxx CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)

add_executable(trajToCluster
    main.cpp
    src/ClusterCalculator.cpp
    src/HysplitOutputNcConverter.cpp
    src/HysplitOutputRenamer.cpp
    src/PairHash.cpp
    src/TextFileReader.cpp
    src/TrajAgeCalculator.cpp
    src/TrajTextFileData.cpp
    src/util.cpp
)

target_compile_features(trajToCluster PRIVATE cxx_std_20)
set_target_properties(trajToCluster PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(trajToCluster
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(trajToCluster
    PRIVATE
        netCDF::netcdf
        netCDF::netcdf-cxx4
        TBB::tbb
        cxxopts::cxxopts
        indicators::indicators
)

if (WIN32)
    target_compile_definitions(trajToCluster PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif ()

if (MSVC)
    target_compile_options(trajToCluster PRIVATE /utf-8 /EHsc)
endif ()

include(GNUInstallDirs)
install(TARGETS trajToCluster RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
