# One test executable per area folder of docs/examples/, built from the
# *_test.cpp files that exist under tests/<area>/ (recursively), plus the
# aggregate target `examples_tests` that builds every area.

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.18.0   # 063de7e9578f82b369302001269680b4b1553359
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
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
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
  add_dependencies(examples_tests ${target})
  # Test names come out as <Suite>.<Test>; the area is added as a label so
  # `ctest -L containers` runs one area.
  gtest_discover_tests(${target}
    PROPERTIES LABELS ${area}
    DISCOVERY_TIMEOUT 60)
endforeach()
