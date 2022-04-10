#include "testutils.hh"

using namespace Tailslide;

ParserRef runConformance(const char* name, bool allow_syntax_errors)
{
  std::string path = __FILE__;
  path.erase(path.find_last_of("\\/"));
  path += "/scripts/";
  path += name;

  ParserRef parser(new ScopedScriptParser());
  Logger *logger = &parser->logger;
  logger->set_check_assertions(true);
  parser->parse_lsl(path);
  LSLScript *script = parser->script;

  if (script == nullptr)
  {
    if (!allow_syntax_errors) {
      std::string message = "script " + path + " completely failed to parse!";
      logger->report();
      FAIL(message);
    }
  } else {
    script->collect_symbols();
    script->link_symbol_tables();
    script->determine_types();
    script->recalculate_reference_data();
    script->propagate_values();
    script->check_best_practices();
    script->validate_globals(true);
    script->check_symbols();
  }

  return parser;
}

void assertNoLintErrors(Logger* logger, const std::string& name) {
  logger->finalize();
  int num_errors = logger->get_errors();
  if (num_errors) {
    logger->report();
    std::string message = "script " + name + " encountered " + std::to_string(num_errors) + " errors during parse!";
    FAIL(message);
  }
}


class ScriptFormatter {
public:
  virtual std::string format(LSLScript *script) const = 0;
};

class ScriptPrettyPrinter: public ScriptFormatter {
public:
  explicit ScriptPrettyPrinter(const PrettyPrintOpts &pretty_opts): pretty_opts(pretty_opts) {};
  virtual std::string format(LSLScript *script) const {
    PrettyPrintVisitor pretty_visitor(pretty_opts);
    script->visit(&pretty_visitor);
    return pretty_visitor.stream.str();
  };
  PrettyPrintOpts pretty_opts;
};

class ScriptTreeDumper: public ScriptFormatter {
public:
  virtual std::string format(LSLScript *script) const {
    TreePrintingVisitor tree_visitor;
    script->visit(&tree_visitor);
    return tree_visitor.stream.str();
  };
};

static void checkStringOutput(
    const char* name,
    const char* expected_prefix,
    const OptimizationContext &ctx,
    void (*massager)(LSLScript* script),
    const ScriptFormatter& formatter
) {
  ParserRef parser = runConformance(name);
  Logger *logger = &parser->logger;
  logger->finalize();
  CHECK(logger->get_errors() == 0);
  if (logger->get_errors()) {
    logger->report();
    // If there are errors at this stage then the AST isn't even guaranteed to be sane.
    return;
  }
  logger->reset();

  if (massager != nullptr)
    massager(parser->script);
  if (ctx)
    parser->script->optimize(ctx);
  parser->script->validate_globals(true);
  parser->script->check_symbols();
  parser->script->get_symbol_table()->set_mangled_names();

  std::string prettified = formatter.format(parser->script);

  std::string path = __FILE__;
  path.erase(path.find_last_of("\\/"));
  path += "/scripts/expected/";
  if (expected_prefix != nullptr) {
    path += expected_prefix;
    path += "/";
  }
  path += name;

  std::ifstream in;
  in.open(path, std::ifstream::in | std::ifstream::binary);
  std::stringstream sstr;
  sstr << in.rdbuf();
  const std::string expected(sstr.str());
  in.close();
  if (expected != prettified) {
    size_t diff_pos;
    for(diff_pos=0; diff_pos < std::min(expected.length(), prettified.length()); ++diff_pos) {
      if (expected[diff_pos] != prettified[diff_pos])
        break;
    }
    std::string msg = expected + "\n\nIS NOT EQUAL TO\n\n" + prettified + "\ndifference at pos " + std::to_string(diff_pos);
    FAIL(msg);
  }
}


void checkPrettyPrintOutput(
        const char* name,
        const OptimizationContext &ctx,
        const PrettyPrintOpts &pretty_opts,
        void (*massager)(LSLScript* script)
) {
  checkStringOutput(
    name,
    nullptr,
    ctx,
    massager,
    ScriptPrettyPrinter(pretty_opts)
  );
}

void checkTreeDumpOutput(
    const char* name,
    const OptimizationContext &ctx,
    void (*massager)(LSLScript* script)
) {
  checkStringOutput(
      name,
      "tree_dump",
      ctx,
      massager,
      ScriptTreeDumper()
  );
}
