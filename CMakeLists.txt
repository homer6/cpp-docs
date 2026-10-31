# cpp-docs test suite: GoogleTest checks of the claims made by the reference
# pages under docs/examples/. See "Running the tests" in README.md.
cmake_minimum_required(VERSION 3.25)

project(cpp_docs_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(tests)
