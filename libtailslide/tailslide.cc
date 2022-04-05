#include <string>

#include "tailslide.hh"

int tailslide_lex_init(void **);

void tailslide_set_in(FILE *, void *);

int tailslide_lex_destroy(void *);

int tailslide_parse(void *);

namespace Tailslide {

ScopedTailslideParser::ScopedTailslideParser(): script(nullptr), ast_sane(true) {
  if (gAllocationManager != nullptr)
    throw "Already have a live allocation manager!";
  gAllocationManager = &_allocation_manager;
  Logger::get()->reset();
}

void ScopedTailslideParser::parse(const std::string &filename) {
  assert(script == nullptr);
  FILE *yyin = fopen(filename.c_str(), "rb");
  if (yyin == nullptr) {
    throw "couldn't open file";
  }
  parse(yyin);
  fclose(yyin);
}

void ScopedTailslideParser::parse(FILE *yyin) {
  assert(script == nullptr);
  void *scanner;

  // initialize flex
  tailslide_lex_init(&scanner);

  // set input file
  tailslide_set_in(yyin, scanner);

  // parse
  tailslide_parse(scanner);

  // clean up flex
  tailslide_lex_destroy(scanner);

  script = gScript;
  ast_sane = !gFatalError;
}

}
