#include <iostream>

#include "fuzz_utils.hh"
#include "tailslide.hh"
#include "passes/tree_simplifier.hh"


static bool initialized = false;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!initialized) {
    Tailslide::tailslide_init_builtins(nullptr);
    initialized = true;
  }

  const char* file = buf_to_file(data, size);
  if (!file) {
    exit(EXIT_FAILURE);
  }

  Tailslide::ScoperScriptParser parser;
  try {
    auto *script = parser.parse_lsl(file);
    if (script) {
      script->collect_symbols();
      script->link_symbol_tables();
      script->determine_types();
      script->recalculate_reference_data();
      script->propagate_values();
      script->check_best_practices();
      if (!parser.logger.get_errors()) {
        Tailslide::OptimizationContext ctx {
          .fold_constants = true,
          .prune_unused_locals = true,
          .prune_unused_globals = true,
          .prune_unused_functions = true,
        };
        script->optimize(ctx);
        script->validate_globals(true);
        script->check_symbols();
      }
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  if (delete_file(file) != 0) {
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
