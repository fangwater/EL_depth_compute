cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(ASAN_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer")
SET(CMAKE_TOOLCHAIN_FILE "/home/fanghz/lib/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(replay)
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_BUILD_TYPE "Debug")
# SET(CMAKE_BUILD_TYPE "Release")
include_directories(SYSTEM "/home/fanghz/lib/vcpkg/installed/x64-linux/include")
find_package(Threads REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(xsimd CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(absl_LIBRARIES
absl::algorithm
absl::base
absl::debugging
absl::flat_hash_map
absl::memory
absl::meta
absl::numeric
absl::str_format
absl::strings
absl::synchronization
absl::time
absl::flags
absl::strings
absl::log
absl::flags_internal
absl::log_initialize
absl::config
absl::base
absl::flags_parse
absl::utility)

add_executable(depth_marker main.cc)
target_compile_options(depth_marker PRIVATE ${ASAN_FLAGS})
target_link_options(depth_marker PRIVATE ${ASAN_FLAGS})
# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     target_compile_options(example PRIVATE -O3 -march=native -fopt-info-vec-optimized)
# elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(example PRIVATE -O3 -march=native -Rpass=loop-vectorize -Rpass-analysis=loop-vectorize -Rpass-missed=loop-vectorize)
# endif ()

# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     target_compile_options(filter_sz PRIVATE -O3 -march=native -fopt-info-vec-optimized)
# elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(filter_sz PRIVATE -O3 -march=native)
# endif ()

target_link_libraries(depth_marker ${absl_LIBRARIES})
target_link_libraries(depth_marker fmt::fmt)
target_link_libraries(depth_marker "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>")
target_link_libraries(depth_marker "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>")
target_link_libraries(depth_marker Folly::folly Folly::folly_deps Folly::follybenchmark Folly::folly_test_util)
target_link_libraries(depth_marker xsimd)
target_link_libraries(depth_marker cppzmq cppzmq-static)
target_link_libraries(depth_marker nlohmann_json::nlohmann_json)


