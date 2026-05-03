cmake_minimum_required(VERSION 3.20)

project(trajToCluster LANGUAGES CXX)

find_package(netCDF REQUIRED)
find_package(netCDFCxx CONFIG QUIET)
find_package(TBB CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)

if(NOT TARGET netCDF::netcdf-cxx4)
    find_program(NetCDFCxx_CONFIG_EXECUTABLE NAMES ncxx4-config)

    if(NetCDFCxx_CONFIG_EXECUTABLE)
        execute_process(
            COMMAND "${NetCDFCxx_CONFIG_EXECUTABLE}" --includedir
            OUTPUT_VARIABLE NetCDFCxx_CONFIG_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND "${NetCDFCxx_CONFIG_EXECUTABLE}" --libdir
            OUTPUT_VARIABLE NetCDFCxx_CONFIG_LIBRARY_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    find_path(NetCDFCxx_INCLUDE_DIR
        NAMES netcdf
        HINTS "${NetCDFCxx_CONFIG_INCLUDE_DIR}"
    )
    find_library(NetCDFCxx_LIBRARY
        NAMES netcdf-cxx4 netcdf_c++4
        HINTS "${NetCDFCxx_CONFIG_LIBRARY_DIR}"
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(netCDFCxx
        REQUIRED_VARS NetCDFCxx_LIBRARY NetCDFCxx_INCLUDE_DIR
    )

    add_library(netCDF::netcdf-cxx4 UNKNOWN IMPORTED)
    set_target_properties(netCDF::netcdf-cxx4 PROPERTIES
        IMPORTED_LOCATION "${NetCDFCxx_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${NetCDFCxx_INCLUDE_DIR}"
    )

    if(TARGET netCDF::netcdf)
        set_target_properties(netCDF::netcdf-cxx4 PROPERTIES
            INTERFACE_LINK_LIBRARIES netCDF::netcdf
        )
    elseif(netCDF_LIBRARIES)
        set_target_properties(netCDF::netcdf-cxx4 PROPERTIES
            INTERFACE_LINK_LIBRARIES "${netCDF_LIBRARIES}"
        )
    endif()
endif()

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
