# One test executable per area folder of docs/examples/, built from the
# *_test.cpp files that exist under tests/<area>/ (recursively), plus the
# aggregate target `examples_tests` that builds every area.

# Conformance flags for cl-style compilers (MSVC and clang-cl; `MSVC` is true
# for both), set for every target here, gtest and the module libraries
# included, so a BMI and its importers see the same flags:
# /permissive- (standard conformance), /Zc:__cplusplus (report the real
# __cplusplus, which some tests compare), /utf-8 (UTF-8 source and execution
# character sets), /EHsc (standard C++ exceptions; CMake adds it already),
# /bigobj (big gtest files exceed the default section count) and, for cl
# only, /Zc:preprocessor (the conforming preprocessor, for __VA_OPT__).
if(MSVC)
  add_compile_options(/permissive- /Zc:__cplusplus /utf-8 /EHsc /bigobj)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/Zc:preprocessor)
  endif()
  set(cppdocs_warnings /W4)   # clang-cl would read -Wall as -Weverything
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(cppdocs_warnings -Wall -Wextra)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.18.0   # 063de7e9578f82b369302001269680b4b1553359
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# MSVC and clang-cl: build gtest against the same dynamic CRT (/MD, /MDd) as
# the tests, CMake's default, instead of gtest's static /MT.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Header-only helpers: output capture and feature-test gating.
add_library(cppdocs_support INTERFACE)
target_include_directories(cppdocs_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cppdocs_support INTERFACE GTest::gtest_main)

# Self-tests for tests/support/.
add_executable(cppdocs_support_tests support/support_test.cpp)
target_link_libraries(cppdocs_support_tests PRIVATE cppdocs_support)
gtest_discover_tests(cppdocs_support_tests PROPERTIES LABELS support DISCOVERY_TIMEOUT 60)

# Reflection (P2996) is behind -freflection on GCC 16; enable it only for the
# file that tests docs/examples/language/expressions/reflection.md (#47).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
  set_source_files_properties(language/expressions/reflection_test.cpp
                              PROPERTIES COMPILE_OPTIONS -freflection)
endif()

add_custom_target(examples_tests)
add_dependencies(examples_tests cppdocs_support_tests)

file(GLOB area_dirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(SORT area_dirs)
foreach(area IN LISTS area_dirs)
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${area} OR area STREQUAL "support")
    continue()
  endif()
  file(GLOB_RECURSE sources CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/${area}/*_test.cpp)
  if(NOT sources)
    continue()
  endif()
  string(REPLACE "-" "_" id "${area}")
  set(target examples_${id}_tests)
  add_executable(${target} ${sources})
  target_link_libraries(${target} PRIVATE cppdocs_support)
  target_compile_options(${target} PRIVATE ${cppdocs_warnings})
  add_dependencies(examples_tests ${target})
  # Test names come out as <Suite>.<Test>; the area is added as a label so
  # `ctest -L containers` runs one area.
  gtest_discover_tests(${target}
    PROPERTIES LABELS ${area}
    DISCOVERY_TIMEOUT 60)
endforeach()

# Named modules (FILE_SET CXX_MODULES) for tests/language/modules/.
if(TARGET examples_language_tests)
  include(language/modules/modules.cmake)
endif()

# <meta> (P2996) needs -freflection on GCC 16; enable it only for the files that
# test docs/examples/metaprogramming/reflection.md (#46) and
# reflection-queries.md (#120).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
  set_source_files_properties(metaprogramming/reflection_test.cpp
                              metaprogramming/reflection-queries_test.cpp
                              PROPERTIES COMPILE_OPTIONS -freflection)
endif()

# <stacktrace> and <debugging> live in libstdc++'s experimental library on
# GCC 16; link it into the diagnostics tests (#46).
if(TARGET examples_diagnostics_tests AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
  target_link_libraries(examples_diagnostics_tests PRIVATE stdc++exp)
endif()
