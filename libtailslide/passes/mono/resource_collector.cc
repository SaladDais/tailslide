#include "resource_collector.hh"

namespace Tailslide {

/// LSL name -> CIL name
std::unordered_map<std::string, std::string> CIL_HANDLER_NAMES {
    {"state_entry", "state_entry"},
    {"state_exit", "state_exit"},
    {"touch_start", "touch_start"},
    {"touch", "touch"},
    {"touch_end", "touch_end"},
    {"collision_start", "collision_start"},
    {"collision", "collision"},
    {"collision_end", "collision_end"},
    {"land_collision_start", "land_collision_start"},
    {"land_collision", "land_collision"},
    {"land_collision_end", "land_collision_end"},
    {"timer", "timer"},
    {"listen", "chat"},
    {"on_rez", "rez"},
    {"sensor", "sensor"},
    {"no_sensor", "no_sensor"},
    {"control", "control"},
    {"money", "money"},
    {"email", "email"},
    {"at_target", "at_target"},
    {"not_at_target", "not_at_target"},
    {"at_rot_target", "at_rot_target"},
    {"not_at_rot_target", "not_at_rot_target"},
    {"run_time_permissions", "run_time_perms"},
    {"changed", "changed"},
    {"attach", "attach"},
    {"dataserver", "dataserver"},
    {"link_message", "link_message"},
    {"moving_start", "moving_start"},
    {"moving_end", "moving_end"},
    {"object_rez", "object_rez"},
    {"remote_data", "remote_event"},
    {"http_response", "http_response"},
    {"http_request", "http_request"},
    {"transaction_result", "transaction_result"},
    {"path_update", "path_update"},
    {"experience_permissions", "experience_permissions"},
    {"experience_permissions_denied", "experience_permissions_denied"}
};

bool MonoResourceVisitor::visit(LSLGlobalFunction *glob_func) {
  auto *sym = glob_func->getSymbol();
  auto *func_sym_data = getSymbolData(sym);
  _mCurrentFunc = func_sym_data;
  // pick up local declarations
  visitChildren(glob_func);
  _mCurrentFunc = nullptr;
  return false;
}

bool MonoResourceVisitor::visit(LSLEventHandler *handler) {
  auto *sym = handler->getSymbol();
  auto *handler_sym_data = getSymbolData(sym);
  _mCurrentFunc = handler_sym_data;
  // pick up local declarations
  visitChildren(handler);
  _mCurrentFunc = nullptr;
  return false;
}

bool MonoResourceVisitor::visit(LSLDeclaration *decl_stmt) {
  auto *sym = decl_stmt->getSymbol();
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
