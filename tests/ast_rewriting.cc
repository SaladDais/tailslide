#include "testutils.hh"

TEST_SUITE_BEGIN("AST Rewriting");

class AddSubbingVisitor: public ASTVisitor {
public:
  bool visit(LLScriptBinaryExpression *expr) override {
    if(expr->get_operation() != '+')
      return true;

    // can only do this for types that allow `-` expressions
    switch (expr->get_type()->get_itype()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
      case LST_VECTOR:
      case LST_QUATERNION:
        break;
      default:
        return true;
    }

    // disconnect the left and right expr nodes from the original parent
    // and attach them to the new expression
    auto *new_expr = gAllocationManager->new_tracked<LLScriptBinaryExpression>(
        (LLScriptExpression*)expr->take_child(0),
        '-',
        (LLScriptExpression*)expr->take_child(1)
    );
    // swap the old expression out with the new one
    LLASTNode::replace_node(expr, new_expr);
    new_expr->visit(this);
    return false;
  };
};

TEST_CASE("simple_expr_replacement.lsl") {
  OptimizationContext ctx{.fold_constants=true};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("simple_expr_replacement.lsl", ctx, pretty_ctx, [](LLScriptScript* script) {
    AddSubbingVisitor visitor;
    script->visit(&visitor);
    // all existing constant values are now potentially dirty, recalculate.
    script->propogate_values();
  });
}

class FancyAddReplacementVisitor: public ASTVisitor {
public:
  bool visit(LLScriptBinaryExpression *expr) override {
    if(expr->get_operation() != '+')
      return true;

    // our injected `whatever()` returns an integer, so we can only replace `+` in
    // integer and floating point contexts
    switch (expr->get_type()->get_itype()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
        break;
      default:
        return true;
    }
    PrettyPrintVisitor pretty_visitor(PrettyPrintOpts {});
    expr->visit(&pretty_visitor);

    char *new_str = gAllocationManager->copy_str(pretty_visitor.stream.str().c_str());

    // replace all addition with a stringified version of the addition expression
    auto new_expr = gAllocationManager->new_tracked<LLScriptFunctionExpression>(
        gAllocationManager->new_tracked<LLScriptIdentifier>(LLScriptType::get(LST_INTEGER), "whatever"),
        gAllocationManager->new_tracked<LLScriptConstantExpression>(
            gAllocationManager->new_tracked<LLScriptStringConstant>(new_str)
        )
    );
    // swap the old expression out with the new one
    LLASTNode::replace_node(expr, new_expr);
    // make sure all the newly added nodes have correct type information
    new_expr->propogate_types();
    new_expr->visit(this);
    return false;
  };
};

TEST_CASE("fancy_expr_replacement.lsl") {
  OptimizationContext ctx{false};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("fancy_expr_replacement.lsl", ctx, pretty_ctx, [](LLScriptScript* script) {
    // pretend we have a builtin called "whatever()" that takes a string and returns an int
    auto *symbol_table = script->get_symbol_table();
    auto func_dec = gAllocationManager->new_tracked<LLScriptFunctionDec>(
        gAllocationManager->new_tracked<LLScriptIdentifier>(LLScriptType::get(LST_STRING), "foobar")
    );
    symbol_table->define(gAllocationManager->new_tracked<LLScriptSymbol>(
        "whatever", LLScriptType::get(LST_INTEGER), SYM_FUNCTION, SYM_BUILTIN, func_dec
    ));
    FancyAddReplacementVisitor visitor;
    script->visit(&visitor);
    // all existing constant values are now potentially dirty, recalculate.
    script->propogate_values();
  });
}

TEST_SUITE_END();
