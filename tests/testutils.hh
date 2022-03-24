#ifndef TESTUTILS_HH
#define TESTUTILS_HH

#include "libsling.hh"
#include "doctest.hh"
#include "passes/pretty_print.hh"

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>



#define SIMPLE_LINT_TEST_CASE(name) TEST_CASE(name) { \
  auto parser = runConformance(name);                 \
  assertNoLintErrors(name);     \
}

using ParserRef = std::unique_ptr<Sling::ScopedSlingParser>;

ParserRef runConformance(const char* name, bool allow_syntax_errors=false);

void assertNoLintErrors(const std::string& name);

void checkPrettyPrintOutput(
        const char* name,
        const Sling::OptimizationContext &ctx,
        const Sling::PrettyPrintOpts &pretty_opts,
        void (*massager)(Sling::LLScriptScript* script) = nullptr
);

void checkTreeDumpOutput(
    const char* name,
    const Sling::OptimizationContext &ctx,
    void (*massager)(Sling::LLScriptScript* script) = nullptr
);

#endif
