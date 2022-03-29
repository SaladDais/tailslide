#include <string>

#include "tailslide.hh"

int yylex_init(void **);

void yyset_in(FILE *, void *);

int yylex_destroy(void *);

int yyparse(void *);

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
  yylex_init(&scanner);

  // set input file
  yyset_in(yyin, scanner);

  // parse
  yyparse(scanner);

  // clean up flex
  yylex_destroy(scanner);

  script = gScript;
  ast_sane = !gFatalError;
}

}
