#include "bytecode_format.hh"
#include "resource_collector.hh"

namespace Tailslide {
bool LSOResourceVisitor::visit(Tailslide::LSLScript *node) {
  // build up symbol data for library functions
  for (auto builtin_val: node->mContext->builtins->getMap()) {
    auto *sym = builtin_val.second;
    if (sym->getSymbolType() != Tailslide::SYM_FUNCTION)
      continue;
    // figure out the size of the parameters
    handleFuncDecl(getSymbolData(sym), sym->getFunctionDecl());
  }
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLGlobalFunction *node) {
  auto *sym = node->getSymbol();
  auto *func_sym_data = getSymbolData(sym);
  func_sym_data->count = _mFuncCount++;
  // enrich function prototype and parameters with sizing information
  handleFuncDecl(func_sym_data, sym->getFunctionDecl());

  _mCurrentFunc = func_sym_data;
  // pick up local declarations
  visitChildren(node);
  _mCurrentFunc = nullptr;
  return false;
}

bool LSOResourceVisitor::visit(Tailslide::LSLGlobalVariable *node) {
  auto *sym = node->getSymbol();
  auto sym_data = getSymbolData(sym);
  // Offset to actual data, type, null terminator for name (empty)
  _mGlobalsOffset += 4 + 1 + 1;
  sym_data->offset = _mGlobalsOffset;
  _mGlobalsOffset += sym_data->size = LSO_TYPE_DATA_SIZES[sym->getIType()];
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLState *node) {
  getSymbolData(node->getSymbol())->count = _mStateCount++;
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLEventHandler *node) {
  auto *sym = node->getSymbol();
  auto *handler_sym_data = getSymbolData(sym);
  // enrich handler prototype and parameters with sizing information
  handleFuncDecl(handler_sym_data, sym->getFunctionDecl());

  _mCurrentFunc = handler_sym_data;
  // pick up local declarations
  visitChildren(node);
  _mCurrentFunc = nullptr;
  // figure out the LSO event handler index for this handler name
  for(size_t handler_idx=0; handler_idx<LSOH_MAX; ++handler_idx) {
    if (!strcmp(sym->getName(), LSO_HANDLER_NAMES[handler_idx])) {
      handler_sym_data->count = (LSOHandlerType)handler_idx;
    }
  }
  assert(handler_sym_data->count);
  return false;
}

bool LSOResourceVisitor::visit(Tailslide::LSLDeclaration *node) {
  auto *sym = node->getSymbol();
  auto *sym_data = getSymbolData(sym);
  sym_data->count = _mCurrentFunc->locals.size();
  sym_data->offset = _mCurrentFunc->size - _mCurrentFunc->offset;
  // local slots are smaller than globals, no overhead for offset to data, type and name.
  _mCurrentFunc->size += sym_data->size = LSO_TYPE_DATA_SIZES[sym->getIType()];
  _mCurrentFunc->locals.push_back(sym->getIType());
  return true;
}

// shared by functions and event handlers
void LSOResourceVisitor::handleFuncDecl(LSOSymbolSizeData *func_sym_data, Tailslide::LSLASTNode *func_decl) {
  if (!func_decl || func_decl->getNodeType() == Tailslide::NODE_NULL)
    return;
  auto *params = (Tailslide::LSLIdentifier *) func_decl->getChildren();
  while (params) {
    auto param_type = params->getIType();
    auto param_size = LSO_TYPE_DATA_SIZES[param_type];
    // may not have a symbol if this is a builtin function!
    auto *param_sym = params->getSymbol();
    if (param_sym) {
      auto *param_sym_data = getSymbolData(param_sym);
      param_sym_data->offset = func_sym_data->offset;
      param_sym_data->size = param_size;
    }

    func_sym_data->offset += param_size;
    func_sym_data->function_args.push_back(param_type);
    params = (Tailslide::LSLIdentifier *) params->getNext();
  }
  // function size is parameters + locals, add the parameter sizes
  // before we start looking at the locals
  func_sym_data->size = func_sym_data->offset;
}

LSOSymbolSizeData *LSOResourceVisitor::getSymbolData(Tailslide::LSLSymbol *sym) {
  auto sym_iter = _mSizeData->find(sym);
  if (sym_iter != _mSizeData->end())
    return &sym_iter->second;
  (*_mSizeData)[sym] = {};
  return &_mSizeData->find(sym)->second;
}

}
