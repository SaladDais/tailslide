#include <string>

#include "tailslide.hh"

int tailslide_lex_init_extra(Tailslide::ScriptContext *, void **);

void tailslide_set_in(FILE *, void *);

int tailslide_lex_destroy(void *);

int tailslide_parse(void *);

namespace Tailslide {

ScopedTailslideParser::ScopedTailslideParser() {
  if (gAllocationManager != nullptr)
    throw "Already have a live allocation manager!";
  gAllocationManager = &_allocation_manager;
  Logger::get()->reset();
  context.logger = Logger::get();
  context.allocator = &_allocation_manager;
  context.script = nullptr;
  context.ast_sane = true;
  _allocation_manager.set_context(&context);
}

LSLScript *ScopedTailslideParser::parse(const std::string &filename) {
  assert(context.script == nullptr);
  FILE *yyin = fopen(filename.c_str(), "rb");
  if (yyin == nullptr) {
    throw "couldn't open file";
  }
  parse(yyin);
  fclose(yyin);
  return script;
}

LSLScript *ScopedTailslideParser::parse(FILE *yyin) {
  assert(context.script == nullptr);
  void *scanner;

  // initialize flex
  tailslide_lex_init_extra(&context, &scanner);

  // set input file
  tailslide_set_in(yyin, scanner);

  // parse
  tailslide_parse(scanner);

  // clean up flex
  tailslide_lex_destroy(scanner);
  script = context.script;
  return script;
}

}
