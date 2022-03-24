cmake_minimum_required(VERSION 3.1)
project(Sling LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(_IGNORED "${CMAKE_C_COMPILER}")

option(SLING_BUILD_CLI "Build CLI" ON)
option(SLING_BUILD_TESTS "Build Tests" ON)
option(SLING_SANITIZE "Use ASAN" OFF)
option(SLING_COVERAGE "Track coverage data in tests" OFF)

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

if (SLING_SANITIZE)
  add_compile_options("-fsanitize=address" "-O1")
  add_link_options("-fsanitize=address")
endif()

if (SLING_COVERAGE)
  if ( NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    message(FATAL_ERROR "Clang must be used to enable coverage tracking, not ${CMAKE_CXX_COMPILER_ID}")
  endif()
  add_compile_options("-fprofile-instr-generate" "-fcoverage-mapping")
  add_link_options("-fprofile-instr-generate")
endif()

add_library(libsling STATIC)
target_sources(libsling PRIVATE
        libsling/allocator.hh
        libsling/ast.hh
        libsling/logger.hh
        libsling/lslmini.hh
        libsling/pretty_print.hh
        libsling/strings.hh
        libsling/symtab.hh
        libsling/tree_base.hh
        libsling/types.hh
        libsling/values.hh
        libsling/libsling.hh

        libsling/allocator.cc
        libsling/ast.cc
        libsling/builtins.cc
        libsling/builtins_txt.cc
        libsling/final_walk.cc
        libsling/logger.cc
        libsling/lslmini.cc
        libsling/operators.cc
        libsling/pretty_print.cc
        libsling/strings.cc
        libsling/symtab.cc
        libsling/types.cc
        libsling/values.cc
        libsling/libsling.cc
)

string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
add_definitions(-DBUILD_DATE="${BUILD_DATE}")
add_definitions(-DVERSION="0.1dev")

# Needed for Bison / Flex
FLEX_TARGET(LSLMiniScanner libsling/lslmini.l ${CMAKE_CURRENT_BINARY_DIR}/lslmini.flex.cc)
BISON_TARGET(LSLMiniParser libsling/lslmini.y ${CMAKE_CURRENT_BINARY_DIR}/lslmini.tab.cc)
ADD_FLEX_BISON_DEPENDENCY(LSLMiniScanner LSLMiniParser)

target_include_directories(libsling PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if (SLING_BUILD_CLI)
  add_executable(sling_cli)
  target_sources(sling_cli PRIVATE
          extern/cxxopt.hh

          sling_cli/main.cc
  )
  target_include_directories(sling_cli PUBLIC ${CMAKE_CURRENT_BINARY_DIR} libsling extern)
  target_link_libraries(sling_cli PUBLIC ${EXTRA_LIBS} libsling)
  set_target_properties(sling_cli PROPERTIES OUTPUT_NAME sling)
endif()

if (SLING_BUILD_TESTS)
  add_executable(sling_test)
  target_sources(sling_test PRIVATE
    extern/doctest.hh
    tests/testutils.hh

    tests/conformance.cc
    tests/ast_rewriting.cc
    tests/testutils.cc
    tests/main.cc
  )
  target_include_directories(sling_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR} libsling extern)
  target_link_libraries(sling_test PUBLIC ${EXTRA_LIBS} libsling)
  set_target_properties(sling_test PROPERTIES OUTPUT_NAME sling-test)
endif()

target_include_directories(libsling PUBLIC libsling)
target_sources(libsling PRIVATE ${FLEX_LSLMiniScanner_OUTPUTS} ${BISON_LSLMiniParser_OUTPUTS})
set_target_properties(libsling PROPERTIES OUTPUT_NAME sling)
