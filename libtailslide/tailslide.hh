#ifndef LIBTAILSLIDE_HH
#define LIBTAILSLIDE_HH

#include <string>

#include "lslmini.hh"

namespace Tailslide {

class ScopedTailslideParser {
public:
    explicit ScopedTailslideParser();
    ScopedTailslideParser(const ScopedTailslideParser&) = delete;
    ~ScopedTailslideParser() {
      assert(&_allocation_manager == gAllocationManager);
      gAllocationManager = nullptr;
      gScript = nullptr;
      gFatalError = false;
      Logger::get()->reset();
    };

    LLScriptScript* script;
    bool ast_sane;
    void parse(FILE *yyin);
    void parse(const std::string &filename);
private:
    ScriptAllocationManager _allocation_manager {};
};

}

#endif