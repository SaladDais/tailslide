#include "bytecode_format.hh"
#include "resource_collector.hh"

namespace Tailslide {
bool LSOResourceVisitor::visit(Tailslide::LSLScript *node) {
  // build up symbol data for library functions
  for (auto builtin_val: node->context->builtins->get_map()) {
    auto *sym = builtin_val.second;
    if (sym->get_symbol_type() != Tailslide::SYM_FUNCTION)
      continue;
    // figure out the size of the parameters
    _handleFuncDecl(_getSymbolData(sym), sym->get_function_decl());
  }
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLGlobalFunction *node) {
  auto *sym = node->get_symbol();
  auto *func_sym_data = _getSymbolData(sym);
  func_sym_data->count = _mFuncCount++;
  // enrich function prototype and parameters with sizing information
  _handleFuncDecl(func_sym_data, sym->get_function_decl());

  _mCurrentFunc = func_sym_data;
  // pick up local declarations
  visit_children(node);
  _mCurrentFunc = nullptr;
  return false;
}

bool LSOResourceVisitor::visit(Tailslide::LSLGlobalVariable *node) {
  auto *sym = node->get_symbol();
  auto sym_data = _getSymbolData(sym);
  // Offset to actual data, type, null terminator for name (empty)
  _mGlobalsOffset += 4 + 1 + 1;
  sym_data->offset = _mGlobalsOffset;
  _mGlobalsOffset += sym_data->size = LSOTypeDataSize[sym->get_itype()];
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLState *node) {
  _getSymbolData(node->get_symbol())->count = _mStateCount++;
  return true;
}

bool LSOResourceVisitor::visit(Tailslide::LSLEventHandler *node) {
  auto *sym = node->get_symbol();
  auto *handler_sym_data = _getSymbolData(sym);
  // enrich handler prototype and parameters with sizing information
  _handleFuncDecl(handler_sym_data, sym->get_function_decl());

  _mCurrentFunc = handler_sym_data;
  // pick up local declarations
  visit_children(node);
  _mCurrentFunc = nullptr;
  // figure out the LSO event handler index for this handler name
  for(size_t handler_idx=0; handler_idx<LSOH_MAX; ++handler_idx) {
    if (!strcmp(sym->get_name(), LSOHandlerNames[handler_idx])) {
      handler_sym_data->count = (LSOHandlerType)handler_idx;
    }
  }
  assert(handler_sym_data->count);
  return false;
}

bool LSOResourceVisitor::visit(Tailslide::LSLDeclaration *node) {
  auto *sym = node->get_symbol();
  auto *sym_data = _getSymbolData(sym);
  sym_data->count = _mCurrentFunc->locals.size();
  sym_data->offset = _mCurrentFunc->size - _mCurrentFunc->offset;
  // local slots are smaller than globals, no overhead for offset to data, type and name.
  _mCurrentFunc->size += sym_data->size = LSOTypeDataSize[sym->get_itype()];
  _mCurrentFunc->locals.push_back(sym->get_itype());
  return true;
}

// shared by functions and event handlers
void LSOResourceVisitor::_handleFuncDecl(LSOSymbolSizeData *func_sym_data, Tailslide::LSLASTNode *func_decl) {
  if (!func_decl || func_decl->get_node_type() == Tailslide::NODE_NULL)
    return;
  auto *params = (Tailslide::LSLIdentifier *) func_decl->get_children();
  while (params) {
    auto param_type = params->get_itype();
    auto param_size = LSOTypeDataSize[param_type];
    // may not have a symbol if this is a builtin function!
    auto *param_sym = params->get_symbol();
    if (param_sym) {
      auto *param_sym_data = _getSymbolData(param_sym);
      param_sym_data->offset = func_sym_data->offset;
      param_sym_data->size = param_size;
    }

    func_sym_data->offset += param_size;
    func_sym_data->function_args.push_back(param_type);
    params = (Tailslide::LSLIdentifier *) params->get_next();
  }
  // function size is parameters + locals, add the parameter sizes
  // before we start looking at the locals
  func_sym_data->size = func_sym_data->offset;
}

LSOSymbolSizeData *LSOResourceVisitor::_getSymbolData(Tailslide::LSLSymbol *sym) {
  auto sym_iter = _mSizeData->find(sym);
  if (sym_iter != _mSizeData->end())
    return &sym_iter->second;
  (*_mSizeData)[sym] = {};
  return &_mSizeData->find(sym)->second;
}

}
