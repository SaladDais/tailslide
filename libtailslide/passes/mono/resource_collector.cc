#include "resource_collector.hh"

namespace Tailslide {
bool MonoResourceVisitor::visit(LSLGlobalFunction *node) {
  auto *sym = node->getSymbol();
  auto *func_sym_data = getSymbolData(sym);
  _mCurrentFunc = func_sym_data;
  // pick up local declarations
  visitChildren(node);
  _mCurrentFunc = nullptr;
  return false;
}

bool MonoResourceVisitor::visit(LSLEventHandler *node) {
  auto *sym = node->getSymbol();
  auto *handler_sym_data = getSymbolData(sym);
  _mCurrentFunc = handler_sym_data;
  // pick up local declarations
  visitChildren(node);
  _mCurrentFunc = nullptr;
  return false;
}

bool MonoResourceVisitor::visit(LSLDeclaration *node) {
  auto *sym = node->getSymbol();
  auto *sym_data = getSymbolData(sym);
  sym_data->index = _mCurrentFunc->locals.size();
  _mCurrentFunc->locals.push_back(sym->getIType());
  return true;
}

MonoSymbolData *MonoResourceVisitor::getSymbolData(LSLSymbol *sym) {
  auto sym_iter = _mSymData->find(sym);
  if (sym_iter != _mSymData->end())
    return &sym_iter->second;
  (*_mSymData)[sym] = {};
  return &_mSymData->find(sym)->second;
}

}
