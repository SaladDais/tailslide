cmake_minimum_required(VERSION 3.6)
project(Tailslide LANGUAGES CXX)
if (MSVC)
  # MSVC requires C++20 be enabled to allow designated initializers
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(_IGNORED "${CMAKE_C_COMPILER}")

option(TAILSLIDE_BUILD_CLI "Build CLI" ON)
option(TAILSLIDE_BUILD_TESTS "Build Tests" ON)
option(TAILSLIDE_BUILD_FUZZER "Build Fuzzer" OFF)
option(TAILSLIDE_SANITIZE "Use ASAN" OFF)
option(TAILSLIDE_COVERAGE "Track coverage data in tests" OFF)


if (TAILSLIDE_BUILD_FUZZER)
  set(TAILSLIDE_SANITIZE ON)
endif ()

# On macOS, search Homebrew for keg-only versions of Bison and Flex. Xcode does
# not provide new enough versions for us to use.
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  execute_process(
          COMMAND brew --prefix bison
          RESULT_VARIABLE BREW_BISON
          OUTPUT_VARIABLE BREW_BISON_PREFIX
          OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_BISON EQUAL 0 AND EXISTS "${BREW_BISON_PREFIX}")
    message(STATUS "Found Bison keg installed by Homebrew at ${BREW_BISON_PREFIX}")
    set(BISON_EXECUTABLE "${BREW_BISON_PREFIX}/bin/bison")
  endif()

  execute_process(
          COMMAND brew --prefix flex
          RESULT_VARIABLE BREW_FLEX
          OUTPUT_VARIABLE BREW_FLEX_PREFIX
          OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_FLEX EQUAL 0 AND EXISTS "${BREW_FLEX_PREFIX}")
    message(STATUS "Found Flex keg installed by Homebrew at ${BREW_FLEX_PREFIX}")
    set(FLEX_EXECUTABLE "${BREW_FLEX_PREFIX}/bin/flex")
  endif()
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(EXTRA_LIBS "")
if(UNIX AND NOT APPLE)
  set(EXTRA_LIBS "stdc++" "rt" "m")
endif()

if (TAILSLIDE_SANITIZE)
  if (TAILSLIDE_BUILD_FUZZER)
    # Everything needs to be compiled with the tracing info for the coverage-based fuzzer
    add_compile_options("-fsanitize=address,fuzzer" "-O1")
  else()
    add_compile_options("-fsanitize=address" "-O1")
  endif()
  # Only the fuzzer harnesses gets linked with -fsanitize=fuzzer, though
  add_link_options("-fsanitize=address")
endif()

if (TAILSLIDE_COVERAGE)
  if ( NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    message(FATAL_ERROR "Clang must be used to enable coverage tracking, not ${CMAKE_CXX_COMPILER_ID}")
  endif()
  add_compile_options("-fprofile-instr-generate" "-fcoverage-mapping")
  add_link_options("-fprofile-instr-generate")
endif()

add_library(libtailslide STATIC
        libtailslide/allocator.cc
        libtailslide/ast.cc
        libtailslide/builtins.cc
        libtailslide/builtins_txt.cc
        libtailslide/logger.cc
        libtailslide/lslmini.cc
        libtailslide/operations.cc
        libtailslide/strings.cc
        libtailslide/symtab.cc
        libtailslide/types.cc
        libtailslide/visitor.cc
        libtailslide/passes/globalexpr_validator.cc
        libtailslide/passes/best_practices.cc
        libtailslide/passes/pretty_print.cc
        libtailslide/passes/symbol_resolution.cc
        libtailslide/passes/type_checking.cc
        libtailslide/passes/tree_print.cc
        libtailslide/passes/tree_simplifier.cc
        libtailslide/passes/values.cc
        libtailslide/tailslide.cc
        )
target_sources(libtailslide PRIVATE
        libtailslide/allocator.hh
        libtailslide/ast.hh
        libtailslide/loctype.hh
        libtailslide/logger.hh
        libtailslide/lslmini.hh
        libtailslide/operations.hh
        libtailslide/strings.hh
        libtailslide/symtab.hh
        libtailslide/types.hh
        libtailslide/visitor.hh
        libtailslide/passes/globalexpr_validator.hh
        libtailslide/passes/best_practices.hh
        libtailslide/passes/pretty_print.hh
        libtailslide/passes/symbol_resolution.hh
        libtailslide/passes/tree_simplifier.hh
        libtailslide/passes/tree_print.hh
        libtailslide/passes/type_checking.hh
        libtailslide/passes/values.hh
        libtailslide/tailslide.hh
)

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
add_definitions(-DBUILD_DATE="${BUILD_DATE}")
add_definitions(-DVERSION="0.1dev")

# Needed for Bison / Flex
FLEX_TARGET(LSLMiniScanner libtailslide/lslmini.l ${CMAKE_CURRENT_BINARY_DIR}/lslmini.flex.cc)
BISON_TARGET(LSLMiniParser libtailslide/lslmini.y ${CMAKE_CURRENT_BINARY_DIR}/lslmini.tab.cc)
ADD_FLEX_BISON_DEPENDENCY(LSLMiniScanner LSLMiniParser)

target_include_directories(libtailslide PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(libtailslide PUBLIC libtailslide)
target_sources(libtailslide PRIVATE ${FLEX_LSLMiniScanner_OUTPUTS} ${BISON_LSLMiniParser_OUTPUTS})
set_target_properties(libtailslide PROPERTIES OUTPUT_NAME tailslide)


install(TARGETS libtailslide DESTINATION lib)
install(DIRECTORY libtailslide/ DESTINATION include/tailslide FILES_MATCHING PATTERN "*.hh")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lslmini.tab.hh DESTINATION include/tailslide)


if (TAILSLIDE_BUILD_CLI)
  add_executable(tailslide_cli tailslide_cli/main.cc)
  target_sources(tailslide_cli PRIVATE
          extern/cxxopt.hh
  )
  target_include_directories(tailslide_cli PUBLIC ${CMAKE_CURRENT_BINARY_DIR} libtailslide extern)
  target_link_libraries(tailslide_cli PUBLIC ${EXTRA_LIBS} libtailslide)
  set_target_properties(tailslide_cli PROPERTIES OUTPUT_NAME tailslide)
endif()


if (TAILSLIDE_BUILD_TESTS)
  add_executable(tailslide_test
          tests/conformance.cc
          tests/ast_rewriting.cc
          tests/testutils.cc
          tests/main.cc
          tests/unit_tests.cc
  )
  target_sources(tailslide_test PRIVATE
    extern/doctest.hh
    tests/testutils.hh
  )
  target_include_directories(tailslide_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR} libtailslide extern)
  target_link_libraries(tailslide_test PUBLIC ${EXTRA_LIBS} libtailslide)
  set_target_properties(tailslide_test PROPERTIES OUTPUT_NAME tailslide-test)
endif()


if (TAILSLIDE_BUILD_FUZZER)
  if ( NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Linux") )
    message(FATAL_ERROR "The libFuzzer harness is only supported under Linux!")
  endif()
  if ( NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    message(FATAL_ERROR "The libFuzzer harness is only supported under Clang, not ${CMAKE_CXX_COMPILER_ID}")
  endif()

  add_executable(tailslide_fuzzer
    fuzz/fuzz.cc
  )
  target_sources(tailslide_fuzzer PRIVATE
    fuzz/fuzz_utils.hh
  )
  target_include_directories(tailslide_fuzzer PUBLIC ${CMAKE_CURRENT_BINARY_DIR} libtailslide)
  target_link_libraries(tailslide_fuzzer PUBLIC ${EXTRA_LIBS} libtailslide "-fsanitize=fuzzer,address")
  set_target_properties(tailslide_fuzzer PROPERTIES OUTPUT_NAME tailslide-fuzzer)
endif()
