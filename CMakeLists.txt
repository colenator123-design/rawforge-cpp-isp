cmake_minimum_required(VERSION 3.20)
project(RawForge VERSION 0.2.0 LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RAWFORGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(rawforge
    src/cfa.cpp
    src/io.cpp
    src/metrics.cpp
    src/pipeline.cpp
    src/synthetic.cpp
)
target_include_directories(rawforge PUBLIC include)
target_link_libraries(rawforge PUBLIC Threads::Threads)
target_compile_options(rawforge PRIVATE -Wall -Wextra -Wpedantic)

if(RAWFORGE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rawforge PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(rawforge PUBLIC -fsanitize=address,undefined)
endif()

add_executable(rawforge_demo apps/demo.cpp)
target_link_libraries(rawforge_demo PRIVATE rawforge)

add_executable(rawforge_benchmark apps/benchmark.cpp)
target_link_libraries(rawforge_benchmark PRIVATE rawforge)

add_executable(rawforge_tune apps/tune.cpp)
target_link_libraries(rawforge_tune PRIVATE rawforge)

add_executable(rawforge_quality_benchmark apps/quality_benchmark.cpp)
target_link_libraries(rawforge_quality_benchmark PRIVATE rawforge)

enable_testing()
add_executable(rawforge_tests tests/test_main.cpp)
target_link_libraries(rawforge_tests PRIVATE rawforge)
add_test(NAME rawforge_tests COMMAND rawforge_tests)

install(TARGETS rawforge rawforge_demo rawforge_benchmark rawforge_tune rawforge_quality_benchmark)
