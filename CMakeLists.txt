cmake_minimum_required(VERSION 3.10)
project(UIED_Pipeline_Benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(benchmark REQUIRED)

include_directories(include)

add_executable(run_main
    src/main.cpp
    src/uied_phase1.cpp
    src/uied_phase2.cpp
)
target_link_libraries(run_main ${OpenCV_LIBS})

add_executable(run_benchmark
    src/benchmark.cpp
    src/uied_phase1.cpp
    src/uied_phase2.cpp
)
target_link_libraries(run_benchmark ${OpenCV_LIBS} benchmark::benchmark pthread)

