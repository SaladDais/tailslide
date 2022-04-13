#pragma once

#include <map>
#include <vector>

#include "visitor.hh"

namespace Tailslide {
struct LSOSymbolSizeData {
  // The end of the parameters for function symbols, offset from the start for everything else
  uint32_t offset = 0;
  // parameters + locals for function symbols, size of the symbol for everything else.
  // this will be == offset for library functions.
  uint32_t size = 0;
  // only really meaningful for states, functions and locals. index starting from 0
  uint64_t count = 0;
  // all locals (if this symbol is a function or event handler)
  std::vector<LST_TYPE> locals{};
  // all arguments (if this symbol is a function or event handler)
  std::vector<LST_TYPE> function_args{};
};

typedef std::map<Tailslide::LSLSymbol *, LSOSymbolSizeData> LSOSymbolSizeDataMap;

// Walks the script, figuring out how much space to reserve for data slots
// and what order to place them in.
class LSOResourceVisitor : public Tailslide::ASTVisitor {
  public:
    explicit LSOResourceVisitor(LSOSymbolSizeDataMap *size_data) : _mSizeData(size_data) {}

  protected:
    bool visit(Tailslide::LSLScript *node) override;
    bool visit(Tailslide::LSLGlobalFunction *node) override;
    bool visit(Tailslide::LSLGlobalVariable *node) override;
    bool visit(Tailslide::LSLState *node) override;
    bool visit(Tailslide::LSLDeclaration *node) override;
    bool visit(Tailslide::LSLEventHandler *node) override;

    LSOSymbolSizeData *_getSymbolData(Tailslide::LSLSymbol *sym);
    void _handleFuncDecl(LSOSymbolSizeData *func_sym_data, LSLASTNode *func_decl);

    uint32_t _mGlobalsOffset = 0;
    uint32_t _mFuncCount = 0;
    uint32_t _mStateCount = 0;
    LSOSymbolSizeData *_mCurrentFunc = nullptr;
    LSOSymbolSizeDataMap *_mSizeData = nullptr;
};
}
