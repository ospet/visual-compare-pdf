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

if(EMSCRIPTEN)
    # Emscripten-specific settings
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    
    # Create the WebAssembly library
    add_executable(pdf_compare_wasm
        src/bindings.cpp
    )
    
    target_include_directories(pdf_compare_wasm
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Set Emscripten flags as a single string
    set(EM_FLAGS
        "-s WASM=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s MODULARIZE=1"
        "-s EXPORT_NAME='createPDFCompare'"
        "-s EXPORTED_RUNTIME_METHODS=['FS']"
        "-s NO_EXIT_RUNTIME=1"
        "-s USE_ZLIB=1"
        "-s USE_LIBPNG=1"
        "-s FORCE_FILESYSTEM=1"
        "--bind"
    )
    
    # Join the flags with spaces
    string(JOIN " " EM_LINK_FLAGS ${EM_FLAGS})
    
    # Set the flags for both compilation and linking
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EM_LINK_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EM_LINK_FLAGS}")

    # Since we can't use Poppler in WASM easily, let's use PDF.js
    target_compile_definitions(pdf_compare_wasm PRIVATE
        USE_PDFJS=1
    )

else()
    # Native build configuration (unchanged)
    # Find required packages
    find_package(unofficial-poppler CONFIG REQUIRED)
    find_package(PNG REQUIRED)

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

    # Create executable
    add_executable(pdf_compare_app
        src/main.cpp
    )

    target_link_libraries(pdf_compare_app
        PRIVATE
            pdf_compare
    ) 

    # Define export macro
    target_compile_definitions(pdf_compare PRIVATE PDF_COMPARE_EXPORTS)
endif()



