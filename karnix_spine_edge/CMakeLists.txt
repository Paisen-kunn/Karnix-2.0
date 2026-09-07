cmake_minimum_required(VERSION 3.10)
project(KarnixSpineEdge CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include paths ─────────────────────────────────────────────────────────────
# Karnix core lives one level up in neural_network/ and medical/
include_directories(
    ${CMAKE_SOURCE_DIR}/..          # exposes neural_network/, medical/, etc.
    ${CMAKE_SOURCE_DIR}/medical     # local SpineCNN / SpineEvaluationUtils
)

# ── Target ────────────────────────────────────────────────────────────────────
add_executable(spine_cli
    spine_cli.c++
    medical/SpineEvaluationUtils.cpp
)

# ── Compiler flags ────────────────────────────────────────────────────────────
target_compile_options(spine_cli PRIVATE
    -Wall
    -Wextra
    -O2
)

# Print offline reminder
message(STATUS "==================================================")
message(STATUS " Karnix Spine Edge")
message(STATUS " Single-file compile:")
message(STATUS "   clang++ -std=gnu++14 spine_cli.c++ -o spine_cli")
message(STATUS "==================================================")
