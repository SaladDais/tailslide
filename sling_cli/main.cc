#include <iostream>
#include <cstdio>

#include "cxxopt.hh"

#include "libsling.hh"
#include "passes/pretty_print.hh"

using namespace Sling;

void version() {
  fprintf(stderr, "Sling CLI tool");
  fprintf(stderr, " built on " __DATE__ "\n");
  fprintf(stderr, " based on https://github.com/pclewis/lslint\n");
}


int main(int argc, char **argv) {
  FILE *yyin = nullptr;
  bool show_tree = false;
  bool pretty_print = true;
  OptimizationContext optim_ctx {};
  PrettyPrintOpts pretty_opts {};

  cxxopts::Options options("sling", "");

  options.add_options("General")
          ("help", "Show this message")
          ("version", "Display the banner and current version of Sling");

  options.add_options("Obfuscation")
          ("stdobf", "Standard obfuscation method - uses all methods with no negative performance impact")
          ("minw", "Minimize whitespace within the script")
          ("mangle_glob", "Mangle and shorten global variable names")
          ("mangle_loc", "Mangle and shorten local variable names")
          ("mangle_func", "Mangle and shorten function names")
          ("show_unmangled", "Put a comment next to instances of mangled identifiers with the original name")
          ("obfuscate_numbers", "Obfuscate numeric literals");

  options.add_options("Optimization / Debug")
          ("O1", "Simple optimizations with no risk or effect on readability")
          ("O2", "Slightly risky optimizations, logic is partially rewritten")
          ("O3", "Risky optimizations that might render script unreadable by humans")
          ("fold_constants", "Simplify the source by performing constant folding")
          ("prune_glob", "Prune unused globals")
          ("prune_loc", "Prune unused locals")
          ("prune_func", "Prune unused functions")
          ("lint", "Only lint the file for errors, don't optimize or pretty print.")
          ("showtree", "Show the AST after optimizations")
          ("check_asserts", "check assert comments and suppress errors based on matches");

  options.add_options()
          ("script", "Input script's filename", cxxopts::value<std::string>());
  options.parse_positional({"script"});
  options.positional_help("<script>");

  cxxopts::ParseResult vm;
  try {
    auto parse_result = options.parse(argc, argv);
    vm = std::move(parse_result);
  } catch (cxxopts::OptionException &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << options.help() << std::endl;
    return 1;
  }

  if (vm.count("help")) {
    std::cerr << options.help() << std::endl;
    return 0;
  }

  if (vm.count("version")) {
    version();
    return 0;
  }

  if (vm.count("script")) {
    std::string filename = vm["script"].as<std::string>();
    yyin = fopen(filename.c_str(), "r");
    if (yyin == nullptr) {
      fprintf(stderr, "couldn't open %s\n", filename.c_str());
      return 1;
    }
  }

  bool check_assertions = vm.count("check_asserts");
  if (vm.count("showtree"))
    show_tree = true;
  if (vm.count("lint")) {
    pretty_print = false;
  } else {
    if (vm.count("minw"))
      pretty_opts.minify_whitespace = true;
    if (vm.count("mangle_glob"))
      pretty_opts.mangle_global_names = true;
    if (vm.count("mangle_loc"))
      pretty_opts.mangle_local_names = true;
    if (vm.count("mangle_func"))
      pretty_opts.mangle_func_names = true;
    if (vm.count("show_unmangled"))
      pretty_opts.show_unmangled = true;
    if (vm.count("obfuscate_numbers"))
      pretty_opts.obfuscate_numbers = true;
    if (vm.count("fold_constants"))
      optim_ctx.fold_constants = true;
    if (vm.count("prune_glob"))
      optim_ctx.prune_unused_globals = true;
    if (vm.count("prune_func"))
      optim_ctx.prune_unused_functions = true;
    if (vm.count("prune_loc"))
      optim_ctx.prune_unused_locals = true;
    if (vm.count("O2")) {
      optim_ctx.prune_unused_globals = true;
      optim_ctx.prune_unused_locals = true;
      optim_ctx.prune_unused_functions = true;
      optim_ctx.fold_constants = true;
    }
    if (vm.count("O3")) {
      optim_ctx.prune_unused_globals = true;
      optim_ctx.prune_unused_locals = true;
      optim_ctx.prune_unused_functions = true;
      optim_ctx.fold_constants = true;
      // the length of global vars / functions and their params has an impact on bytecode size
      pretty_opts.mangle_global_names = true;
      pretty_opts.mangle_func_names = true;
      pretty_opts.show_unmangled = true;
      // stops `-1` from being treated as `unary_minus(1)`
      pretty_opts.obfuscate_numbers = true;
    }
    if (vm.count("stdobf")) {
      optim_ctx.prune_unused_globals = true;
      optim_ctx.prune_unused_locals = true;
      optim_ctx.prune_unused_functions = true;
      optim_ctx.fold_constants = true;
      pretty_opts.mangle_global_names = true;
      pretty_opts.mangle_func_names = true;
      pretty_opts.mangle_local_names = true;
      pretty_opts.show_unmangled = false;
      pretty_opts.obfuscate_numbers = true;
      pretty_opts.minify_whitespace = true;
    }
  }
  sling_init_builtins(nullptr);
  // set up the allocator and logger
  ScopedSlingParser sling_parser;

  if (check_assertions)
    Logger::get()->set_check_assertions(true);

  sling_parser.parse(yyin);
  if (yyin != nullptr)
    fclose(yyin);
  LLScriptScript* script = sling_parser.script;

  if (script) {
    LOG(LOG_INFO, nullptr, "Script parsed, collecting symbols");
    script->collect_symbols();
    script->link_symbol_tables();
    LOG(LOG_INFO, nullptr, "Propogating types");
    script->propogate_types();
    script->recalculate_reference_data();
    script->propogate_values();
    script->final_pre_walk();

    if (check_assertions) {
      Logger::get()->filter_assert_errors();
      Logger::get()->set_check_assertions(false);
    }
    // Don't try to optimize if we have a possibly broken tree
    if (!Logger::get()->get_errors()) {
      script->optimize(optim_ctx);

      // do this last since symbol usage may change
      // when rewriting the tree
      script->check_symbols();
      if (pretty_print) {
        script->get_symbol_table()->set_mangled_names();

        PrettyPrintVisitor print_visitor(pretty_opts);
        script->visit(&print_visitor);
        std::cout << print_visitor.stream.str() << "\n";
      }
    } else {
      script->check_symbols();
    }
    Logger::get()->report();
    if (show_tree) {
      LOG(LOG_INFO, nullptr, "Tree:");
      TreePrintingVisitor visitor;
      script->visit(&visitor);
      std::cout << visitor.stream.str();
    }
  } else {
    Logger::get()->report();
  }

  return Logger::get()->get_errors();
}
