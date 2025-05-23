cmake_minimum_required(VERSION 3.20)
project(Browser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

find_package(Qt6 COMPONENTS Core Gui Widgets Network Svg SvgWidgets REQUIRED)

set(SPECTRA_INCLUDE_DIR "D:/Cpp/Libraries/spectra-1.1.0/include" CACHE PATH "Path to Spectra's include directory")
if(NOT EXISTS "${SPECTRA_INCLUDE_DIR}/Spectra/SymEigsSolver.h")
    message(FATAL_ERROR "Spectra headers not found in ${SPECTRA_INCLUDE_DIR}. Please set SPECTRA_INCLUDE_DIR correctly.")
endif()
include_directories(${SPECTRA_INCLUDE_DIR})

find_package(Threads REQUIRED)

find_package(Eigen3 REQUIRED)
find_package(CURL REQUIRED)

set(PROJECT_RESOURCES
        resources.qrc
)

add_executable(Browser
        main.cpp
        mainwindow.cpp
        mainwindow.h
        searchenginecore.cpp
        searchenginecore.h
        ${PROJECT_RESOURCES}
)

set_target_properties(Browser PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        WIN32_EXECUTABLE TRUE
)

target_include_directories(Browser PRIVATE "D:/MSys/mingw64/include/qt6/QtSvg")

target_link_libraries(Browser PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Svg
        Qt6::SvgWidgets
        Threads::Threads
        stdc++fs
        Eigen3::Eigen
        CURL::libcurl
)

add_executable(wikipedia_data_processor
        wikipedia_data_processor.cpp
)

target_include_directories(wikipedia_data_processor PRIVATE
        Eigen3::Eigen
)

target_link_libraries(wikipedia_data_processor PRIVATE
        CURL::libcurl
        Threads::Threads
        stdc++fs
        Eigen3::Eigen
        Qt6::Core
)