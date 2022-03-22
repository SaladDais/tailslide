#ifndef TESTUTILS_HH
#define TESTUTILS_HH

#include "libsling.hh"
#include "doctest.hh"
#include "pretty_print.hh"

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>



#define SIMPLE_LINT_TEST_CASE(name) TEST_CASE(name) { \
  auto parser = runConformance(name);                 \
  assertNoLintErrors(name);     \
}

using ParserRef = std::unique_ptr<ScopedSlingParser>;

ParserRef runConformance(const char* name, bool allow_syntax_errors=false);

void assertNoLintErrors(const std::string& name);

void checkPrettyPrintOutput(
        const char* name,
        const OptimizationContext &ctx,
        const PrettyPrintOpts &pretty_opts,
        void (*massager)(LLScriptScript* script) = nullptr
);

void checkTreeDumpOutput(
    const char* name,
    const OptimizationContext &ctx,
    void (*massager)(LLScriptScript* script) = nullptr
);

#endif
