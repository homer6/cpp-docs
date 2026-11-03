# Fuzz targets (#307), built only with CPPDOCS_FUZZ=ON and Clang: libFuzzer
# plus ASan and UBSan (ASan only on Apple, where Homebrew LLVM's objects
# with UBSan don't link with the system linker). Run one with
#   build/<preset>/fuzz/fuzz_parse_port -max_total_time=60 <copy of fuzz/corpus/parse_port>
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
  message(FATAL_ERROR "CPPDOCS_FUZZ needs Clang with libFuzzer (-fsanitize=fuzzer)")
endif()
if(APPLE)
  set(cppdocs_fuzz_san fuzzer,address)
else()
  set(cppdocs_fuzz_san fuzzer,address,undefined)
endif()
add_custom_target(fuzz_targets)
foreach(t parse_port payload nesting)
  add_executable(fuzz_${t} fuzz_${t}.cpp)
  target_compile_options(fuzz_${t} PRIVATE -g -O1 -fsanitize=${cppdocs_fuzz_san} -fno-sanitize-recover=all)
  target_link_options(fuzz_${t} PRIVATE -fsanitize=${cppdocs_fuzz_san})
  add_dependencies(fuzz_targets fuzz_${t})
endforeach()
message(STATUS "fuzz: targets fuzz_parse_port, fuzz_payload, fuzz_nesting (-fsanitize=${cppdocs_fuzz_san})")
