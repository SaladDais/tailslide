#include <algorithm>
#include <vector>       // vector::iterator
#include <set>
#include <string>

#include "lslmini.hh"
#include "symtab.hh"

namespace Tailslide {

void LSLSymbolTable::define(LSLSymbol *symbol) {
  symbols.insert(SensitiveSymbolMap::value_type(symbol->get_name(), symbol));
  DEBUG(
    LOG_DEBUG_SPAM,
    NULL,
    "defined symbol: %d %s %s\n",
    symbol->get_symbol_type(),
    symbol->get_type() ? symbol->get_type()->get_node_name() : "!!!NULL!!!",
    symbol->get_name()
  );
}

LSLSymbol *LSLSymbolTable::lookup(const char *name, LSLSymbolType type) {
  auto sym_range = symbols.equal_range(name);
  for (auto it = sym_range.first; it != sym_range.second; ++it) {
    if (type == SYM_ANY || type == it->second->get_symbol_type())
      return it->second;
  }
  return nullptr;
}

void LSLSymbolTable::check_symbols() {
  for (auto &symbol: symbols) {
    LSLSymbol *sym = symbol.second;
    if (sym->get_sub_type() != SYM_BUILTIN && sym->get_sub_type() != SYM_EVENT_PARAMETER &&
        sym->get_references() <= 1) {
      ERROR(IN(sym), W_DECLARED_BUT_NOT_USED, LSLSymbol::get_type_name(sym->get_symbol_type()), sym->get_name());
    }
  }
}

bool LSLSymbolTable::remove(LSLSymbol *symbol) {
  for (auto iter = symbols.begin(); iter != symbols.end(); ++iter) {
    if (iter->second == symbol) {
      symbols.erase(iter);
      return true;
    }
  }
  return false;
}

void LSLSymbolTable::reset_reference_data() {
  for (auto &symbol: symbols) {
    symbol.second->reset_tracking();
  }
  for (auto &desc_table: desc_tables) {
    desc_table->reset_reference_data();
  }
}

/* Oddly enough, using shorter names in globals save bytecode space. */
void LSLSymbolTable::set_mangled_names() {
  int seq = 0;
  auto table_mangler = [seq, this](SensitiveSymbolMap &node_symbols) mutable {
    std::set<std::string> key_names;

    for (auto &it: node_symbols) {
      key_names.insert(it.first);
    }
    std::vector<std::string> sorted_key_names(key_names.begin(), key_names.end());

    for (auto &key_name: sorted_key_names) {
    auto range = node_symbols.equal_range(key_name.c_str());
      for (auto &symbol = range.first; symbol != range.second; ++symbol) {
        LSLSymbol *sym = symbol->second;
        // can't rename events or builtin names, obviously!
        if (sym->get_symbol_type() == SYM_EVENT || sym->get_sub_type() == SYM_BUILTIN)
          continue;

        char *mangled_id = gAllocationManager->alloc(30);
        while (true) {
          snprintf(mangled_id, 30, "_%x", seq++);
          // Make sure this name isn't already in use
          if (!this->lookup(mangled_id, SYM_ANY)) {
            sym->set_mangled_name(mangled_id);
            break;
          }
        }
      }
    }
  };

  table_mangler(symbols);
  for (auto &desc_table: desc_tables) {
    table_mangler(desc_table->symbols);
  }
}

void LSLSymbolTable::register_subtable(LSLSymbolTable *table) {
  if (table != this)
    desc_tables.push_back(table);
}

void LSLSymbolTable::unregister_subtable(LSLSymbolTable *table) {
  auto found = std::find(desc_tables.begin(), desc_tables.end(), table);
  if (found != desc_tables.end())
    desc_tables.erase(found);
}
}
