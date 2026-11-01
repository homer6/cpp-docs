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

project(cpp_docs_tests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
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
