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

# Sanitizers (#263): CPPDOCS_SANITIZE=address,undefined (or thread, or
# address) builds everything, gtest and the module libraries included, with
# the sanitizer, so a BMI and its importers match. UB stops the test that hit
# it (-fno-sanitize-recover=all). MSVC and clang-cl take only "address"
# (/fsanitize=address). The test presets set ASAN_OPTIONS and UBSAN_OPTIONS.
set(CPPDOCS_SANITIZE "" CACHE STRING "Sanitizers for the suite, e.g. address,undefined or thread (empty: none)")
if(CPPDOCS_SANITIZE)
  if(MSVC)
    if(NOT CPPDOCS_SANITIZE STREQUAL "address")
      message(FATAL_ERROR "CPPDOCS_SANITIZE: MSVC and clang-cl support only 'address', not '${CPPDOCS_SANITIZE}'")
    endif()
    add_compile_options(/fsanitize=address /Zi)
    add_compile_definitions(_DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(_san -fsanitize=${CPPDOCS_SANITIZE} -fno-omit-frame-pointer -g)
    if(CPPDOCS_SANITIZE MATCHES "undefined")
      list(APPEND _san -fno-sanitize-recover=all)
    endif()
    add_compile_options(${_san})
    add_link_options(-fsanitize=${CPPDOCS_SANITIZE})
  else()
    message(FATAL_ERROR "CPPDOCS_SANITIZE: no sanitizer support for ${CMAKE_CXX_COMPILER_ID}")
  endif()
  add_compile_definitions(CPPDOCS_SANITIZE="${CPPDOCS_SANITIZE}")
  message(STATUS "tests: sanitizers on: ${CPPDOCS_SANITIZE}")
endif()

# Hardening (#305): CPPDOCS_HARDEN=ON builds the suite the way a hardened
# release ships: standard-library hardening (picked below by the library
# actually in use), stack protection, zero-initialized automatic variables,
# _FORTIFY_SOURCE on glibc in optimized builds, PIE, full RELRO and a
# non-executable stack on ELF, control-flow protection where the target has
# it, and /sdl /guard:cf on cl-style compilers. Each flag is probed first, so
# a toolchain without one skips it. The flags actually used are cached as
# CPPDOCS_HARDEN_FLAGS for tools/ctest_to_json.py. The default comes from the
# CPPDOCS_HARDEN environment variable, so CI jobs set it without extra
# plumbing; the *-hardened presets set it directly.
if(DEFINED ENV{CPPDOCS_HARDEN} AND NOT "$ENV{CPPDOCS_HARDEN}" MATCHES "^(|0|OFF|off|false)$")
  set(cppdocs_harden_default ON)
else()
  set(cppdocs_harden_default OFF)
endif()
option(CPPDOCS_HARDEN "Build the suite with library hardening and hardening compiler/linker flags" ${cppdocs_harden_default})
set(cppdocs_harden_flags "")
if(CPPDOCS_HARDEN)
  include(CheckCXXCompilerFlag)
  include(CheckLinkerFlag)
  include(CheckPIESupported)
  set(CMAKE_REQUIRED_QUIET ON)
  function(cppdocs_try_compile_flag flag)
    string(MAKE_C_IDENTIFIER "CPPDOCS_HAS_FLAG_${flag}" var)
    check_cxx_compiler_flag("${flag}" ${var})
    if(${var})
      add_compile_options("${flag}")
      set(cppdocs_harden_flags ${cppdocs_harden_flags} "${flag}" PARENT_SCOPE)
    endif()
  endfunction()
  function(cppdocs_try_link_flag flag)
    string(MAKE_C_IDENTIFIER "CPPDOCS_HAS_LINK_${flag}" var)
    check_linker_flag(CXX "${flag}" ${var})
    if(${var})
      add_link_options("${flag}")
      set(cppdocs_harden_flags ${cppdocs_harden_flags} "${flag}" PARENT_SCOPE)
    endif()
  endfunction()
  if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      cppdocs_try_compile_flag(/sdl)
    endif()
    cppdocs_try_compile_flag(/guard:cf)
    cppdocs_try_link_flag(/guard:cf)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64")
      cppdocs_try_link_flag(/CETCOMPAT)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    cppdocs_try_compile_flag(-fstack-protector-strong)
    cppdocs_try_compile_flag(-ftrivial-auto-var-init=zero)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      cppdocs_try_compile_flag(-fstack-clash-protection)
    endif()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
      cppdocs_try_compile_flag(-fcf-protection=full)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
      # GCC on macOS: -mbranch-protection=pac-ret (part of "standard") breaks
      # C++ exception unwinding, so every throw aborts (vendor bug, #327).
      if(NOT (APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        cppdocs_try_compile_flag(-mbranch-protection=standard)
      endif()
    endif()
    # _FORTIFY_SOURCE needs optimization and is a glibc feature (Apple's SDK
    # turns its own on by default).
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_compile_options(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3)
      list(APPEND cppdocs_harden_flags -D_FORTIFY_SOURCE=3)
    endif()
    if(NOT APPLE AND NOT WIN32)
      cppdocs_try_link_flag(-Wl,-z,relro)
      cppdocs_try_link_flag(-Wl,-z,now)
      cppdocs_try_link_flag(-Wl,-z,noexecstack)
    endif()
  endif()
  check_pie_supported(OUTPUT_VARIABLE cppdocs_pie_msg LANGUAGES CXX)
  if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # targets below, gtest included, are PIE
    list(APPEND cppdocs_harden_flags PIE)
  endif()
  add_compile_definitions(CPPDOCS_HARDEN=1)
endif()

# The standard the suite is built as (26, or 23 where CMake knows no C++26 flag:
# MSVC, GCC 13), so tests about __cplusplus can tell what was requested (#209).
add_compile_definitions(CPPDOCS_CXX_STANDARD=${CMAKE_CXX_STANDARD})

# GTest links Threads::Threads, which carries -pthread as a compile option
# wherever FindThreads uses the flag (FreeBSD, for one; not glibc 2.34+,
# macOS or Windows). A module's BMI and its importers must be compiled alike,
# or Clang rejects the BMI ("POSIX thread support was disabled in PCH file
# but is currently enabled", -Wmodule-file-config-mismatch), and CMake builds
# the BMIs, the std module's included, without the importer's usage
# requirements. So compile everything with -pthread there.
find_package(Threads REQUIRED)
get_target_property(_cppdocs_thread_opts Threads::Threads INTERFACE_COMPILE_OPTIONS)
if(_cppdocs_thread_opts MATCHES "-pthread")
  string(APPEND CMAKE_CXX_FLAGS " -pthread")
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

# Runtime libraries some toolchains need beyond the default link, found by
# try-linking a small program without and then with each candidate, and
# linked into every test target (through cppdocs_support) only when needed:
# - libatomic: std::atomic<T> for a T too big to be lock-free calls
#   __atomic_is_lock_free, __atomic_load, ... (GCC 13-15 and Clang with
#   libstdc++ or libc++ on Linux, GCC on FreeBSD, MinGW).
# - libstdc++'s experimental library: <stacktrace> (GCC 14+), <debugging> and,
#   on Windows, std::print's terminal output (vprint_unicode) live in
#   libstdc++exp; GCC 13 has <stacktrace> in libstdc++_libbacktrace.
# A library that doesn't exist fails the try-link too, so nothing is added
# and the link error stays visible.
include(CheckCXXSourceCompiles)
function(cppdocs_find_link_library out source)
  set(CMAKE_REQUIRED_QUIET ON)
  check_cxx_source_compiles("${source}" ${out}_NONE)
  if(${out}_NONE)
    set(${out} "" PARENT_SCOPE)
    return()
  endif()
  foreach(lib IN LISTS ARGN)
    string(MAKE_C_IDENTIFIER "${lib}" lib_id)
    set(CMAKE_REQUIRED_LIBRARIES ${lib})
    check_cxx_source_compiles("${source}" ${out}_${lib_id})
    if(${out}_${lib_id})
      set(${out} ${lib} PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${out} "" PARENT_SCOPE)
endfunction()

# The standard library the suite builds against, from its own macros, cached
# as CPPDOCS_STDLIB for tools/ctest_to_json.py (#190). Guessing from the OS
# got MSYS2 CLANG64 wrong: Clang on Windows there uses libc++.
set(cppdocs_stdlib unknown)
foreach(pair "libc++;_LIBCPP_VERSION" "libstdc++;__GLIBCXX__" "MS STL;_MSVC_STL_VERSION")
  list(GET pair 0 name)
  list(GET pair 1 macro)
  string(MAKE_C_IDENTIFIER "CPPDOCS_STDLIB_IS_${macro}" var)
  set(CMAKE_REQUIRED_QUIET ON)
  check_cxx_source_compiles("#include <version>\n#ifndef ${macro}\n#error no\n#endif\nint main() { return 0; }" ${var})
  if(${var})
    set(cppdocs_stdlib "${name}")
    break()
  endif()
endforeach()
set(CPPDOCS_STDLIB "${cppdocs_stdlib}" CACHE INTERNAL "Standard library detected from its macros")

# Standard-library hardening for CPPDOCS_HARDEN (#305), by the library in use:
# libstdc++'s assertions, libc++'s fast mode (its documented production mode),
# the MS STL's hardening. None changes the ABI.
if(CPPDOCS_HARDEN)
  if(cppdocs_stdlib STREQUAL "libstdc++")
    add_compile_definitions(_GLIBCXX_ASSERTIONS)
    list(APPEND cppdocs_harden_flags -D_GLIBCXX_ASSERTIONS)
  elseif(cppdocs_stdlib STREQUAL "libc++")
    add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
    list(APPEND cppdocs_harden_flags -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
  elseif(cppdocs_stdlib STREQUAL "MS STL")
    add_compile_definitions(_MSVC_STL_HARDENING=1)
    list(APPEND cppdocs_harden_flags /D_MSVC_STL_HARDENING=1)
  endif()
  list(JOIN cppdocs_harden_flags " " cppdocs_harden_joined)
  set(CPPDOCS_HARDEN_FLAGS "${cppdocs_harden_joined}" CACHE INTERNAL "Hardening flags the suite was built with")
  message(STATUS "tests: hardening on: ${cppdocs_harden_joined}")
else()
  set(CPPDOCS_HARDEN_FLAGS "" CACHE INTERNAL "Hardening flags the suite was built with")
endif()
message(STATUS "cpp-docs: standard library ${CPPDOCS_STDLIB}")

cppdocs_find_link_library(cppdocs_atomic_lib [[
#include <atomic>
struct Big { char bytes[64]; };
int main() {
  std::atomic<Big> big{};
  Big b = big.load();
  big.store(b);
  return big.is_lock_free() ? 1 : 0;
}
]] atomic)

cppdocs_find_link_library(cppdocs_stdexp_lib [[
#include <version>
#include <cstdio>
#include <string>
#ifdef __cpp_lib_stacktrace
#include <stacktrace>
#endif
#ifdef __cpp_lib_print
#include <print>
#endif
#ifdef __cpp_lib_debugging
#include <debugging>
#endif
int main(int argc, char**) {
  int r = 0;
#ifdef __cpp_lib_stacktrace
  r += static_cast<int>(std::to_string(std::stacktrace::current()).size());
#endif
#ifdef __cpp_lib_print
  if (argc > 1) std::print(stdout, "{}", argc);
#endif
#ifdef __cpp_lib_debugging
  r += std::is_debugger_present() ? 1 : 0;
#endif
  return r == -1;
}
]] stdc++exp stdc++_libbacktrace)

foreach(lib IN ITEMS ${cppdocs_stdexp_lib} ${cppdocs_atomic_lib})
  message(STATUS "cpp-docs: linking the tests with -l${lib}")
  target_link_libraries(cppdocs_support INTERFACE ${lib})
endforeach()

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
  if(area STREQUAL "security")
    # The fuzz targets' parsers and saved corpus, replayed as tests (#307).
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/fuzz)
    target_compile_definitions(${target} PRIVATE CPPDOCS_FUZZ_DIR="${PROJECT_SOURCE_DIR}/fuzz")
  endif()
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
