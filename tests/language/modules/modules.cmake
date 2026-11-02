# Named modules for tests/language/modules/ (issue #55), included from
# tests/CMakeLists.txt after the per-area test executables exist.
#
# The module units in units/ are built into cppdocs_modules with
# FILE_SET CXX_MODULES; the *_test.cpp files that import them are scanned.
# What can't be built here makes the tests skip with the reason, via
# CPPDOCS_NAMED_MODULES_SKIP / CPPDOCS_IMPORT_STD_SKIP.

set(_mod_dir ${CMAKE_CURRENT_SOURCE_DIR}/language/modules)
set(_mod_tests
    ${_mod_dir}/modules_test.cpp
    ${_mod_dir}/building_test.cpp)

# --- Can this configuration build named modules? (cmake-cxxmodules(7)) -----
set(_skip "")
if(CMAKE_VERSION VERSION_LESS 3.28)
  set(_skip "CMake ${CMAKE_VERSION} has no FILE_SET CXX_MODULES (needs 3.28)")
elseif(NOT CMAKE_GENERATOR MATCHES "^Ninja")
  set(_skip "generator '${CMAKE_GENERATOR}': this suite builds modules with Ninja only")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(_skip "Apple Clang ${CMAKE_CXX_COMPILER_VERSION}: no clang-scan-deps, so CMake can't scan it for modules")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
  set(_skip "GCC ${CMAKE_CXX_COMPILER_VERSION}: CMake scans modules with GCC 14 and newer")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  set(_skip "Clang ${CMAKE_CXX_COMPILER_VERSION}: CMake scans modules with Clang 16 and newer")
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|MSVC")
  set(_skip "compiler ${CMAKE_CXX_COMPILER_ID}: CMake can't scan it for modules")
endif()

if(_skip)
  set_property(TARGET examples_language_tests APPEND PROPERTY
               COMPILE_DEFINITIONS "CPPDOCS_NAMED_MODULES_SKIP=\"${_skip}\""
                                   "CPPDOCS_IMPORT_STD_SKIP=\"${_skip}\"")
  message(STATUS "tests/language/modules: named modules off: ${_skip}")
  return()
endif()

set(_units ${_mod_dir}/units)
add_library(cppdocs_modules STATIC)
target_sources(cppdocs_modules
  PRIVATE
    ${_units}/math.cpp                     # implementation units: ordinary sources
    ${_units}/geo.cpp
    ${_units}/counter.cpp
  PUBLIC
    FILE_SET CXX_MODULES BASE_DIRS ${_units} FILES
      ${_units}/math.cppm
      ${_units}/shapes.cppm
      ${_units}/config.cppm
      ${_units}/geo-types.cppm             # every partition provides a BMI,
      ${_units}/geo-ops.cppm               # so internal ones go here too
      ${_units}/geo-detail.cppm
      ${_units}/geo.cppm
      ${_units}/counter.cppm
      ${_units}/token.cppm
      ${_units}/base.cppm
      ${_units}/wrapper.cppm)
# The top-level cmake_minimum_required(3.25) leaves CMP0155 OLD, so ordinary
# sources aren't scanned by default: opt in for the implementation units and
# for the tests that import modules.
set_property(TARGET cppdocs_modules PROPERTY CXX_SCAN_FOR_MODULES ON)
set_source_files_properties(${_mod_tests} PROPERTIES CXX_SCAN_FOR_MODULES ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # GCC 15 and 16: "sorry, unimplemented: private module fragment".
  target_sources(cppdocs_modules PUBLIC FILE_SET CXX_MODULES FILES ${_units}/tally.cppm)
  set_property(TARGET examples_language_tests APPEND PROPERTY
               COMPILE_DEFINITIONS CPPDOCS_PRIVATE_MODULE_FRAGMENT)
endif()

target_link_libraries(examples_language_tests PRIVATE cppdocs_modules)
set_property(TARGET examples_language_tests APPEND PROPERTY
             COMPILE_DEFINITIONS CPPDOCS_NAMED_MODULES)

# --- import std ---------------------------------------------------------
# CMake 4.5 provides std to every scanned C++20+ source (but needs GCC 16 on
# macOS: libstdc++ 15 lacks the std module initializer there). CMake 3.30-4.4
# need the experimental gate set before project() (top-level CMakeLists.txt)
# and CXX_MODULE_STD on the target; CMAKE_CXX_COMPILER_IMPORT_STD lists the
# standards the toolchain supports.
set(_std_skip "")
set(_std_level ${CMAKE_CXX_STANDARD})
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.5)
  if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    set(_std_skip "CMake ${CMAKE_VERSION}: import std needs GCC 16 or newer on macOS")
  endif()
elseif(NOT DEFINED CMAKE_EXPERIMENTAL_CXX_IMPORT_STD)
  set(_std_skip "CMake ${CMAKE_VERSION}: import std is experimental before 4.5, and the top-level CMakeLists.txt sets the gate only for 4.3 and 4.4 (the versions tested)")
elseif(CMAKE_CXX_STDLIB_MODULES_JSON AND NOT (IS_ABSOLUTE "${CMAKE_CXX_STDLIB_MODULES_JSON}"
                                             AND EXISTS "${CMAKE_CXX_STDLIB_MODULES_JSON}"))
  # The compiler's -print-file-name didn't find the file (Homebrew LLVM keeps
  # libc++.modules.json in lib/c++/), and CMake would fail at generate time.
  set(_std_skip "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}: '${CMAKE_CXX_STDLIB_MODULES_JSON}' not found; pass -DCMAKE_CXX_STDLIB_MODULES_JSON=<path> to build import std")
elseif(NOT _std_level IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
  string(REPLACE ";" "," _levels "${CMAKE_CXX_COMPILER_IMPORT_STD}")  # no ';' in a definition
  set(_std_skip "CMake ${CMAKE_VERSION} with ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}: no import std for C++${_std_level} (CMAKE_CXX_COMPILER_IMPORT_STD='${_levels}')")
endif()

if(_std_skip)
  set_property(TARGET examples_language_tests APPEND PROPERTY
               COMPILE_DEFINITIONS "CPPDOCS_IMPORT_STD_SKIP=\"${_std_skip}\"")
  message(STATUS "tests/language/modules: import std off: ${_std_skip}")
else()
  add_library(cppdocs_modules_std STATIC)
  target_sources(cppdocs_modules_std PUBLIC
    FILE_SET CXX_MODULES BASE_DIRS ${_units} FILES
      ${_units}/stdlib_demo.cppm
      ${_units}/stdcompat_demo.cppm)
  # CMake 4.5 provides std only to targets with CXX_SCAN_FOR_MODULES on; with
  # CMP0155 OLD a module file set alone isn't enough (seen with 4.5.0-rc2).
  set_property(TARGET cppdocs_modules_std PROPERTY CXX_SCAN_FOR_MODULES ON)
  if(CMAKE_VERSION VERSION_LESS 4.5)
    set_property(TARGET cppdocs_modules_std examples_language_tests PROPERTY CXX_MODULE_STD ON)
  endif()
  target_link_libraries(examples_language_tests PRIVATE cppdocs_modules_std)
  set_property(TARGET examples_language_tests APPEND PROPERTY
               COMPILE_DEFINITIONS CPPDOCS_IMPORT_STD)
  message(STATUS "tests/language/modules: import std on")
endif()
