#include "testutils.hh"

ParserRef runConformance(const char* name, bool allow_syntax_errors)
{
  std::string path = __FILE__;
  path.erase(path.find_last_of("\\/"));
  path += "/scripts/";
  path += name;

  ParserRef parser(new ScopedSlingParser());
  Logger::get()->set_check_assertions(true);
  parser->parse(path);
  LLScriptScript *script = parser->script;

  if (script == nullptr)
  {
    if (!allow_syntax_errors) {
      std::string message = "script " + path + " completely failed to parse!";
      FAIL(message);
    }
  } else {
    script->collect_symbols();
    script->link_symbol_tables();
    script->propogate_types();
    script->recalculate_reference_data();
    script->propogate_values();
    script->final_pre_walk();
    script->check_symbols();
  }

  return parser;
}

void assertNoLintErrors(const std::string& name) {
  // need to get the heinous global state out of here eventually.
  Logger::get()->finalize();
  int num_errors = Logger::get()->get_errors();
  if (num_errors) {
    Logger::get()->report();
    std::string message = "script " + name + " encountered " + std::to_string(num_errors) + " errors during parse!";
    FAIL(message);
  }
}


class ScriptFormatter {
public:
  virtual std::string format(LLScriptScript *script) const = 0;
};

class ScriptPrettyPrinter: public ScriptFormatter {
public:
  explicit ScriptPrettyPrinter(const PrettyPrintOpts &pretty_opts): pretty_opts(pretty_opts) {};
  virtual std::string format(LLScriptScript *script) const {
    PrettyPrintVisitor pretty_visitor(pretty_opts);
    script->visit(&pretty_visitor);
    return pretty_visitor.stream.str();
  };
  PrettyPrintOpts pretty_opts;
};

class ScriptTreeDumper: public ScriptFormatter {
public:
  virtual std::string format(LLScriptScript *script) const {
    TreePrintingVisitor tree_visitor;
    script->visit(&tree_visitor);
    return tree_visitor.stream.str();
  };
};

static void checkStringOutput(
    const char* name,
    const char* expected_prefix,
    const OptimizationContext &ctx,
    void (*massager)(LLScriptScript* script),
    const ScriptFormatter& formatter
) {
  ParserRef parser = runConformance(name);
  Logger::get()->finalize();
  CHECK(Logger::get()->get_errors() == 0);
  if (Logger::get()->get_errors()) {
    Logger::get()->report();
    // If there are errors at this stage then the AST isn't even guaranteed to be sane.
    return;
  }
  Logger::get()->reset();

  if (massager != nullptr)
    massager(parser->script);
  if (ctx)
    parser->script->optimize(ctx);
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
        void (*massager)(LLScriptScript* script)
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
    void (*massager)(LLScriptScript* script)
) {
  checkStringOutput(
      name,
      "tree_dump",
      ctx,
      massager,
      ScriptTreeDumper()
  );
}
