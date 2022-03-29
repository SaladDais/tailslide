#ifndef TESTUTILS_HH
#define TESTUTILS_HH

#include "tailslide.hh"
#include "doctest.hh"
#include "passes/pretty_print.hh"
#include "passes/tree_print.hh"
#include "passes/tree_simplifier.hh"

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>



#define SIMPLE_LINT_TEST_CASE(name) TEST_CASE(name) { \
  auto parser = runConformance(name);                 \
  assertNoLintErrors(name);     \
}

using ParserRef = std::unique_ptr<Tailslide::ScopedTailslideParser>;

ParserRef runConformance(const char* name, bool allow_syntax_errors=false);

void assertNoLintErrors(const std::string& name);

void checkPrettyPrintOutput(
        const char* name,
        const Tailslide::OptimizationContext &ctx,
        const Tailslide::PrettyPrintOpts &pretty_opts,
        void (*massager)(Tailslide::LLScriptScript* script) = nullptr
);

void checkTreeDumpOutput(
    const char* name,
    const Tailslide::OptimizationContext &ctx,
    void (*massager)(Tailslide::LLScriptScript* script) = nullptr
);

#endif
