#ifndef LIBSLING_HH
#define LIBSLING_HH

#include <string>

#include "lslmini.hh"

namespace Sling {

class ScopedSlingParser {
public:
    explicit ScopedSlingParser();
    ScopedSlingParser(const ScopedSlingParser&) = delete;
    ~ScopedSlingParser() {
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