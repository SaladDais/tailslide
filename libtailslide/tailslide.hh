#ifndef LIBTAILSLIDE_HH
#define LIBTAILSLIDE_HH

#include <string>

// This is the header for the public API, don't let any internal
// macros or whatnot leak out to consumers.
#ifndef SHOW_TAILSLIDE_INTERNALS
# define HIDE_TAILSLIDE_INTERNALS
#endif

#include "lslmini.hh"

namespace Tailslide {

class ScoperScriptParser {
  public:
    ScoperScriptParser(): allocator(), logger(&allocator) {
      context.allocator = &allocator;
      context.logger = &logger;
    };
    ScriptAllocator allocator;
    Logger logger;
    LSLScript *script = nullptr;
    bool ast_sane = false;

    LSLScript *parse_lsl(FILE *yyin);
    LSLScript *parse_lsl(const std::string &filename);
  protected:
    ScriptContext context;
};

}

#endif