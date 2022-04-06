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
  assertNoLintErrors(&parser->logger, name);     \
}

using ParserRef = std::unique_ptr<Tailslide::ScoperScriptParser>;

ParserRef runConformance(const char* name, bool allow_syntax_errors=false);

void assertNoLintErrors(Tailslide::Logger *logger, const std::string& name);

void checkPrettyPrintOutput(
        const char* name,
        const Tailslide::OptimizationContext &ctx,
        const Tailslide::PrettyPrintOpts &pretty_opts,
        void (*massager)(Tailslide::LSLScript* script) = nullptr
);

void checkTreeDumpOutput(
    const char* name,
    const Tailslide::OptimizationContext &ctx,
    void (*massager)(Tailslide::LSLScript* script) = nullptr
);

#endif
