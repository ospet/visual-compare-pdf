cmake_minimum_required(VERSION 3.10)
project(pdf_compare)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify vcpkg root directory - adjust this path to match your setup
set(VCPKG_ROOT "C:/dev/vcpkg")
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-windows")

# Make vcpkg work better with Visual Studio
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# Specify that we want 64-bit builds
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")

# Find required packages
find_package(unofficial-poppler CONFIG REQUIRED)
find_package(PNG REQUIRED)

# Create library
add_library(pdf_compare STATIC
    src/pdf_compare.cpp
)

target_include_directories(pdf_compare
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pdf_compare
    PRIVATE
        ${VCPKG_INSTALLED_DIR}/lib/poppler.lib
        ${VCPKG_INSTALLED_DIR}/lib/poppler-cpp.lib
        PNG::PNG
)

# Define export macro
target_compile_definitions(pdf_compare PRIVATE PDF_COMPARE_EXPORTS)

# Create executable
add_executable(pdf_compare_app
    src/main.cpp
)

target_link_libraries(pdf_compare_app
    PRIVATE
        pdf_compare
) 