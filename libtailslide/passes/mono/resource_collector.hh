#pragma once

#include <map>
#include <vector>

#include "../../visitor.hh"

namespace Tailslide {
struct MonoSymbolData {
  // Index of the given variable within the locals (not for parameters or globals)
  uint32_t index = 0;
  // all locals (if this symbol is a function or event handler)
  std::vector<LSLIType> locals{};
};

typedef std::map<Tailslide::LSLSymbol *, MonoSymbolData> MonoSymbolDataMap;

// Walks the script, figuring out how much space to reserve for data slots
// and what order to place them in.
class MonoResourceVisitor : public ASTVisitor {
  public:
    explicit MonoResourceVisitor(MonoSymbolDataMap *sym_data) : _mSymData(sym_data) {}

  protected:
    bool visit(Tailslide::LSLGlobalFunction *node) override;
    bool visit(Tailslide::LSLEventHandler *node) override;
    bool visit(Tailslide::LSLDeclaration *node) override;
    // not relevant
    bool visit(Tailslide::LSLExpression *node) override { return false; };

    MonoSymbolData *getSymbolData(Tailslide::LSLSymbol *sym);

    MonoSymbolData *_mCurrentFunc = nullptr;
    MonoSymbolDataMap *_mSymData = nullptr;
};

extern std::unordered_map<std::string, std::string> CIL_HANDLER_NAMES;

}
