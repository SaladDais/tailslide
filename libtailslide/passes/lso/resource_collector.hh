#pragma once

#include <map>
#include <vector>

#include "bytecode_format.hh"
#include "../../visitor.hh"

namespace Tailslide {
struct LSOSymbolData {
  // The end of the parameters for function symbols, offset from the start for everything else
  uint32_t offset = 0;
  // parameters + locals for function symbols, size of the symbol for everything else.
  // this will be == offset for library functions.
  uint32_t size = 0;
  // index within some collection starting from 0
  uint64_t index = 0;
  // defined event handlers in sort order, if this is a state
  std::set<LSOHandlerType> handlers{};
  // all locals (if this symbol is a function or event handler)
  std::vector<LSLIType> locals{};
  // all arguments (if this symbol is a function or event handler)
  std::vector<LSLIType> function_args{};
};

typedef std::map<Tailslide::LSLSymbol *, LSOSymbolData> LSOSymbolDataMap;

// Walks the script, figuring out how much space to reserve for data slots
// and what order to place them in.
class LSOResourceVisitor : public Tailslide::ASTVisitor {
  public:
    explicit LSOResourceVisitor(LSOSymbolDataMap *sym_data) : _mSymData(sym_data) {}

  protected:
    bool visit(Tailslide::LSLScript *node) override;
    bool visit(Tailslide::LSLGlobalFunction *node) override;
    bool visit(Tailslide::LSLGlobalVariable *node) override;
    bool visit(Tailslide::LSLState *node) override;
    bool visit(Tailslide::LSLDeclaration *node) override;
    bool visit(Tailslide::LSLEventHandler *node) override;
    // not relevant
    bool visit(Tailslide::LSLExpression *node) override {return false;};

    LSOSymbolData *getSymbolData(Tailslide::LSLSymbol *sym);
    void handleFuncDecl(LSOSymbolData *func_sym_data, LSLASTNode *func_decl);

    uint32_t _mGlobalsOffset = 0;
    uint32_t _mFuncCount = 0;
    uint32_t _mStateCount = 0;
    LSOSymbolData *_mCurrentFunc = nullptr;
    LSOSymbolData *_mCurrentState = nullptr;
    LSOSymbolDataMap *_mSymData = nullptr;
};
}
