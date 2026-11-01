# cpp-docs test suite: GoogleTest checks of the claims made by the reference
# pages under docs/examples/. See "Running the tests" in README.md.
cmake_minimum_required(VERSION 3.25)

# `import std;` is experimental in CMake 3.30-4.4: each release has its own
# gate UUID (Help/dev/experimental.rst in that release), and it must be set
# before project(). CMake 4.5 drops the gate. tests/language/modules/ skips
# its import std tests, with the reason, on versions not listed here.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.3 AND CMAKE_VERSION VERSION_LESS 4.4)
  set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4 AND CMAKE_VERSION VERSION_LESS 4.5)
  set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
endif()

# The generic presets (gcc, clang, clang-libcxx, msvc, clang-cl, mingw) set
# CPPDOCS_TOOLCHAIN and leave the compiler to the environment: CXX, or
# -DCMAKE_CXX_COMPILER=... If neither is given, use the toolchain's usual
# driver name from PATH. After project(), check the compiler really is that
# toolchain, so a CI job can't silently test the wrong one (e.g. AppleClang
# under the gcc preset).
set(CPPDOCS_TOOLCHAIN "" CACHE STRING
    "Toolchain a generic preset expects: gcc, clang, clang-libcxx, msvc, clang-cl or mingw (empty: no check)")
if(CPPDOCS_TOOLCHAIN AND NOT CMAKE_CXX_COMPILER AND "$ENV{CXX}" STREQUAL "")
  set(_cppdocs_default_cxx_gcc g++)
  set(_cppdocs_default_cxx_clang clang++)
  set(_cppdocs_default_cxx_clang-libcxx clang++)
  set(_cppdocs_default_cxx_msvc cl)
  set(_cppdocs_default_cxx_clang-cl clang-cl)
  set(_cppdocs_default_cxx_mingw g++)
  if(DEFINED _cppdocs_default_cxx_${CPPDOCS_TOOLCHAIN})
    set(CMAKE_CXX_COMPILER ${_cppdocs_default_cxx_${CPPDOCS_TOOLCHAIN}})
  endif()
endif()

project(cpp_docs_tests VERSION 0.2.1 LANGUAGES CXX)

if(CPPDOCS_TOOLCHAIN)
  set(_id "${CMAKE_CXX_COMPILER_ID}")
  set(_ok FALSE)
  if(CPPDOCS_TOOLCHAIN STREQUAL "gcc")
    if(_id STREQUAL "GNU" AND NOT WIN32)
      set(_ok TRUE)
    endif()
  elseif(CPPDOCS_TOOLCHAIN MATCHES "^clang(-libcxx)?$")
    if(_id MATCHES "^(Clang|AppleClang)$" AND NOT MSVC)
      set(_ok TRUE)
    endif()
  elseif(CPPDOCS_TOOLCHAIN STREQUAL "msvc")
    if(_id STREQUAL "MSVC")
      set(_ok TRUE)
    endif()
  elseif(CPPDOCS_TOOLCHAIN STREQUAL "clang-cl")
    if(_id STREQUAL "Clang" AND MSVC)
      set(_ok TRUE)
    endif()
  elseif(CPPDOCS_TOOLCHAIN STREQUAL "mingw")
    if(_id STREQUAL "GNU" AND MINGW)
      set(_ok TRUE)
    endif()
  else()
    message(FATAL_ERROR "CPPDOCS_TOOLCHAIN='${CPPDOCS_TOOLCHAIN}': expected gcc, clang, clang-libcxx, msvc, clang-cl or mingw")
  endif()
  if(NOT _ok)
    message(FATAL_ERROR "Preset toolchain '${CPPDOCS_TOOLCHAIN}' but the compiler is "
                        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
                        "(${CMAKE_CXX_COMPILER}). Set CXX or -DCMAKE_CXX_COMPILER "
                        "to a ${CPPDOCS_TOOLCHAIN} compiler and configure a fresh build "
                        "directory (cmake --fresh --preset ${CPPDOCS_TOOLCHAIN}).")
  endif()
  message(STATUS "cpp-docs toolchain ${CPPDOCS_TOOLCHAIN}: ${CMAKE_CXX_COMPILER_ID} "
                 "${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
endif()

set(CMAKE_CXX_STANDARD 26)
# CMake (through 4.3) knows no C++26 flag for MSVC (cl), only C++23, which it
# maps to /std:c++latest, the newest mode cl has. clang-cl gets
# -std:c++latest for 26 from CMake itself.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_STANDARD 23)
endif()
# Older compilers (GCC 13, for one) have no C++26 flag CMake knows about, so
# configuring would fail outright. Build them as C++23 instead: the tests gate
# every newer feature on its feature-test macro, so those tests skip.
if(NOT CMAKE_CXX_STANDARD EQUAL 23 AND NOT "cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  message(STATUS "cpp-docs: CMake knows no C++26 flag for ${CMAKE_CXX_COMPILER_ID} "
                 "${CMAKE_CXX_COMPILER_VERSION}; building as C++23 (C++26 tests skip)")
  set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Every release has notes: this fails if docs/releases/v<PROJECT_VERSION>.md
# is missing, so the version above and the notes can't drift apart.
add_test(NAME release_notes_exist
         COMMAND ${CMAKE_COMMAND} -E cat
                 ${CMAKE_SOURCE_DIR}/docs/releases/v${PROJECT_VERSION}.md)
set_tests_properties(release_notes_exist PROPERTIES LABELS release)
add_subdirectory(tests)
