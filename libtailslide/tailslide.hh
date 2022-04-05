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

class ScopedTailslideParser {
public:
    explicit ScopedTailslideParser();
    ScopedTailslideParser(const ScopedTailslideParser&) = delete;
    ~ScopedTailslideParser() {
      assert(&_allocation_manager == gAllocationManager);
      gAllocationManager = nullptr;
      Logger::get()->reset();
    };

    LSLScript *parse(FILE *yyin);
    LSLScript *parse(const std::string &filename);
    LSLScript *script = nullptr;
private:
    ScriptAllocationManager _allocation_manager {};
    ScriptContext context {};
};

}

#endif