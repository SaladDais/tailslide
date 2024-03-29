#include "tailslide.hh"
#include "doctest.hh"
#include "passes/pretty_print.hh"
#include "testutils.hh"

using namespace Tailslide;

TEST_SUITE_BEGIN("Lint");

TEST_CASE("bad_globals.lsl") {
  auto parser = runConformance("bad_globals.lsl", true);
  // syntax error due to bad tokens
  CHECK(parser->logger.getErrors() == 2);
}
SIMPLE_LINT_TEST_CASE("camera_demo.lsl")
SIMPLE_LINT_TEST_CASE("check_all_return.lsl")
SIMPLE_LINT_TEST_CASE("color-change.lsl")
SIMPLE_LINT_TEST_CASE("compound_assignment.lsl")
SIMPLE_LINT_TEST_CASE("constants.lsl")
SIMPLE_LINT_TEST_CASE("constprop.lsl")
SIMPLE_LINT_TEST_CASE("declaration_expressions.lsl")
SIMPLE_LINT_TEST_CASE("duplicate_labels.lsl")
SIMPLE_LINT_TEST_CASE("error1.lsl")
SIMPLE_LINT_TEST_CASE("events.lsl")
SIMPLE_LINT_TEST_CASE("forloops.lsl")
SIMPLE_LINT_TEST_CASE("fpinc.lsl")
SIMPLE_LINT_TEST_CASE("fwcheck1.lsl")
SIMPLE_LINT_TEST_CASE("hex.lsl")
SIMPLE_LINT_TEST_CASE("illegal_cast.lsl")
SIMPLE_LINT_TEST_CASE("irc-4.lsl")
// technically this should not pass because LL's impl has different
// rules for labels shadowing vars from outer scopes than it does
// for vars shadowing functions from outer scopes. It should not
// be able to resolve the `foo` bar for the `llOwnerSay()` call.
SIMPLE_LINT_TEST_CASE("label_shadowing.lsl")
SIMPLE_LINT_TEST_CASE("libhttpdb.lsl")
SIMPLE_LINT_TEST_CASE("lsl_conformance.lsl")
SIMPLE_LINT_TEST_CASE("many_statements.lsl")
SIMPLE_LINT_TEST_CASE("mms_player.lsl")
SIMPLE_LINT_TEST_CASE("nested_lists.lsl")
SIMPLE_LINT_TEST_CASE("parser_abuse.lsl")

#ifndef _WIN32
SIMPLE_LINT_TEST_CASE("parserstackdepth2.lsl")
#endif

TEST_CASE("parserstackdepth3.lsl") {
  auto parser = runConformance("parserstackdepth3.lsl", true);
  CHECK(parser->script == nullptr);
  // error about stack depth, then syntax error due to unexpected end.
  CHECK(parser->logger.getErrors() == 2);
}
SIMPLE_LINT_TEST_CASE("print_expression.lsl")
TEST_CASE("print_no_shadowing.lsl") {
  auto parser = runConformance("print_no_shadowing.lsl", true);
  // syntax error due to unexpected keyword
  CHECK(parser->logger.getErrors() == 2);
}
SIMPLE_LINT_TEST_CASE("pathological_expression.lsl")
SIMPLE_LINT_TEST_CASE("print_type_bug.lsl")
SIMPLE_LINT_TEST_CASE("rvalue_assignments.lsl")
SIMPLE_LINT_TEST_CASE("scope1.lsl")
SIMPLE_LINT_TEST_CASE("scope2.lsl")
SIMPLE_LINT_TEST_CASE("scope3.lsl")
SIMPLE_LINT_TEST_CASE("scope4.lsl")
SIMPLE_LINT_TEST_CASE("streamcomment.lsl")
TEST_CASE("strict_salist.lsl") {
  auto parser = runConformance("strict_salist.lsl");
  // assert should be satisfied by validating globals with LSO semantics
  parser->script->validateGlobals(false);
  parser->logger.finalize();
  CHECK(parser->logger.getErrors() == 0);
}
SIMPLE_LINT_TEST_CASE("lenient_salist.lsl")
SIMPLE_LINT_TEST_CASE("test1.lsl")
SIMPLE_LINT_TEST_CASE("test2.lsl")
SIMPLE_LINT_TEST_CASE("unixtime.lsl")
SIMPLE_LINT_TEST_CASE("various_globals.lsl")
SIMPLE_LINT_TEST_CASE("vconst.lsl")
SIMPLE_LINT_TEST_CASE("void_return.lsl")
SIMPLE_LINT_TEST_CASE("warning1.lsl")
SIMPLE_LINT_TEST_CASE("warning2.lsl")
SIMPLE_LINT_TEST_CASE("weirdscope-jump.lsl")
SIMPLE_LINT_TEST_CASE("xytext1.2.lsl")
SIMPLE_LINT_TEST_CASE("ll_global_rules.lsl")

SIMPLE_LINT_TEST_CASE("bugs/0001.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0002.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0003.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0004.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0005.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0006.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0007.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0008.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0009.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0010.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0011.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0012.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0013.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0014.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0015.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0016.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0017.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0018.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0019.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0020.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0021.lsl")
SIMPLE_LINT_TEST_CASE("bugs/0022.lsl")
SIMPLE_LINT_TEST_CASE("bugs/unary_minus_glob_bad_sym.lsl")
SIMPLE_LINT_TEST_CASE("bugs/typecast_builtin.lsl")
SIMPLE_LINT_TEST_CASE("tltp/browser.lsl")
SIMPLE_LINT_TEST_CASE("tltp/exporter.lsl")
SIMPLE_LINT_TEST_CASE("tltp/server.lsl")


TEST_CASE("type_error_no_assert.lsl") {
  auto parser = runConformance("type_error_no_assert.lsl");
  parser->logger.setCheckAssertions(false);
  parser->logger.finalize();
  CHECK(parser->logger.getErrors() == 1);
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("Optimization");


TEST_CASE("xytext1.2.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("xytext1.2.lsl", ctx, pretty_ctx);
}

TEST_CASE("irc-4.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("irc-4.lsl", ctx, pretty_ctx);
}

TEST_CASE("parser_abuse.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("parser_abuse.lsl", ctx, pretty_ctx);
}

TEST_CASE("scope3.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("scope3.lsl", ctx, pretty_ctx);
}

TEST_CASE("mms_player.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  pretty_ctx.mangle_func_names = true;
  pretty_ctx.mangle_global_names = true;
  pretty_ctx.mangle_local_names = true;
  pretty_ctx.show_unmangled = true;
  checkPrettyPrintOutput("mms_player.lsl", ctx, pretty_ctx);
}

#ifndef _WIN32
TEST_CASE("parserstackdepth2.lsl") {
  OptimizationOptions ctx{};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("parserstackdepth2.lsl", ctx, pretty_ctx);
}
#endif

TEST_CASE("forloops.lsl") {
  OptimizationOptions ctx{};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("forloops.lsl", ctx, pretty_ctx);
}

TEST_CASE("constprop.lsl") {
  OptimizationOptions ctx {
    .fold_constants = true,
    .prune_unused_locals = true,
    .prune_unused_globals = true,
    .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("constprop.lsl", ctx, pretty_ctx);
}

TEST_CASE("tltp/browser.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("tltp/browser.lsl", ctx, pretty_ctx);
}

TEST_CASE("tltp/exporter.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("tltp/exporter.lsl", ctx, pretty_ctx);
}

TEST_CASE("tltp/server.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("tltp/server.lsl", ctx, pretty_ctx);
}

TEST_CASE("fpinc.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("fpinc.lsl", ctx, pretty_ctx);
}

TEST_CASE("key_inlining.lsl") {
  OptimizationOptions ctx {
      .fold_constants = true,
      .prune_unused_locals = true,
      .prune_unused_globals = true,
      .prune_unused_functions = true,
  };
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("key_inlining.lsl", ctx, pretty_ctx);
}

TEST_CASE("num_literals.lsl") {
  OptimizationOptions ctx {};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("num_literals.lsl", ctx, pretty_ctx);
}

TEST_CASE("vector_ambiguity.lsl") {
  OptimizationOptions ctx {};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("vector_ambiguity.lsl", ctx, pretty_ctx);
}

TEST_CASE("infinity_repr.lsl") {
  OptimizationOptions ctx {};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("infinity_repr.lsl", ctx, pretty_ctx);
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("Tree Printer");

TEST_CASE("error1.lsl") {
  checkTreeDumpOutput("error1.lsl", OptimizationOptions{});
}

TEST_CASE("tltp/exporter.lsl") {
  checkTreeDumpOutput("tltp/exporter.lsl", OptimizationOptions{});
}

TEST_CASE("pathological_expression.lsl") {
  checkTreeDumpOutput("pathological_expression.lsl", OptimizationOptions{});
}

TEST_CASE("jump_kinds.lsl") {
  checkTreeDumpOutput("jump_kinds.lsl", OptimizationOptions{});
}

static const char *SIMPLE_SCRIPT_BYTES = "default{state_entry(){}}";

TEST_CASE("Parse script buffer") {
  ParserRef parser(new ScopedScriptParser(nullptr));
  auto script = parser->parseLSLBytes(SIMPLE_SCRIPT_BYTES, (int)strlen(SIMPLE_SCRIPT_BYTES));
  CHECK_NE(nullptr, script);
  CHECK_EQ(0, parser->logger.getErrors());
}

TEST_SUITE_END();
