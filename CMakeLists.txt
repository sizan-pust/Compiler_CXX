cmake_minimum_required(VERSION 3.16)
project(MiniCompiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output the executable to the project root so Python can find it easily
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(compiler
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/semantic.cpp
    src/icg.cpp
    src/optimizer.cpp
    src/codegen.cpp
)

target_include_directories(compiler PRIVATE src)

# Enable warnings
if (MSVC)
    target_compile_options(compiler PRIVATE /W4)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra)
endif()
