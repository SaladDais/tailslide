#include <string>

#include "tailslide.hh"

int tailslide_lex_init_extra(Tailslide::ScriptContext *, void **);

void tailslide_set_in(FILE *, void *);

int tailslide_lex_destroy(void *);

namespace Tailslide {

extern LSLSymbolTable gBuiltinsSymbolTable;

ScopedScriptParser::ScopedScriptParser(LSLSymbolTable *builtins) : logger(&allocator), table_manager(&allocator) {
  context.allocator = &allocator;
  context.logger = &logger;
  context.table_manager = &table_manager;
  if (builtins)
    context.builtins = builtins;
  else
    context.builtins = &gBuiltinsSymbolTable;
}

// make sure we don't leak an FH if we throw
class FileCloser {
  public:
    explicit FileCloser(FILE *file): _mFile(file) {};
    ~FileCloser() {fclose(_mFile);};
    FILE *_mFile;
};

LSLScript *ScopedScriptParser::parseLSL(const std::string &filename) {
  // can only be used to parse a single script.
  assert(!script);
  FILE *yyin = fopen(filename.c_str(), "rb");
  if (yyin == nullptr) {
    throw "couldn't open file";
  }
  FileCloser closer(yyin);
  auto result = parseLSL(yyin);
  return result;
}

LSLScript *ScopedScriptParser::parseLSL(FILE *yyin) {
  assert(!script);
  void *scanner;
  // ScopedScriptParser owns the allocator and context instance because we can't
  // reasonably re-use Allocator instances with our current model of having
  // it magically pass along the current script context.
  allocator.setContext(&context);

  // initialize flex
  tailslide_lex_init_extra(&context, &scanner);

  // set input file
  tailslide_set_in(yyin, scanner);

  // parse
  context.parsing = true;
  tailslide_parse(scanner);
  context.parsing = false;

  // clean up flex
  tailslide_lex_destroy(scanner);
  ast_sane = context.ast_sane;
  script = context.script;

  return script;
}

}
