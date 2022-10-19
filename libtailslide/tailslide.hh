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

struct ScopedScriptParser {
    explicit ScopedScriptParser(LSLSymbolTable *builtins);
    ScriptAllocator allocator {};
    Logger logger;
    LSLScript *script = nullptr;
    bool ast_sane = false;
    ScriptContext context;
    LSLSymbolTableManager table_manager;

    LSLScript *parseLSLFile(FILE *yyin);
    LSLScript *parseLSLFile(const std::string &filename);
    LSLScript *parseLSLBytes(const char *buf, int buf_len);

  protected:
    void initScanner();
    void parseInternal();
};

}

#endif