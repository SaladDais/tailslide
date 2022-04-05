#include "testutils.hh"

using namespace Tailslide;

TEST_SUITE_BEGIN("AST Rewriting");

class AddSubbingVisitor: public ASTVisitor {
public:
  bool visit(LSLBinaryExpression *expr) override {
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
    auto *new_expr = expr->context->allocator->new_tracked<LSLBinaryExpression>(
        (LSLExpression*)expr->take_child(0),
        '-',
        (LSLExpression*)expr->take_child(1)
    );
    // swap the old expression out with the new one
    LSLASTNode::replace_node(expr, new_expr);
    new_expr->visit(this);
    return false;
  };
};

TEST_CASE("simple_expr_replacement.lsl") {
  OptimizationContext ctx{.fold_constants=true};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("simple_expr_replacement.lsl", ctx, pretty_ctx, [](LSLScript* script) {
    AddSubbingVisitor visitor;
    script->visit(&visitor);
    // all existing constant values are now potentially dirty, recalculate.
    script->propagate_values();
  });
}

class FancyAddReplacementVisitor: public ASTVisitor {
public:
  bool visit(LSLBinaryExpression *expr) override {
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

    auto *allocator = expr->context->allocator;
    char *new_str = allocator->copy_str(pretty_visitor.stream.str().c_str());

    // replace all addition with a stringified version of the addition expression
    auto new_expr = allocator->new_tracked<LSLFunctionExpression>(
        allocator->new_tracked<LSLIdentifier>(LSLType::get(LST_INTEGER), "whatever"),
        allocator->new_tracked<LSLConstantExpression>(
            allocator->new_tracked<LSLStringConstant>(new_str)
        )
    );
    // swap the old expression out with the new one
    LSLASTNode::replace_node(expr, new_expr);
    // make sure all the newly added nodes have correct type information
    new_expr->determine_types();
    new_expr->visit(this);
    return false;
  };
};

TEST_CASE("fancy_expr_replacement.lsl") {
  OptimizationContext ctx{false};
  PrettyPrintOpts pretty_ctx {};
  checkPrettyPrintOutput("fancy_expr_replacement.lsl", ctx, pretty_ctx, [](LSLScript* script) {
    // pretend we have a builtin called "whatever()" that takes a string and returns an int
    auto *symbol_table = script->get_symbol_table();
    auto *allocator = script->context->allocator;
    auto func_dec = allocator->new_tracked<LSLFunctionDec>(
        allocator->new_tracked<LSLIdentifier>(LSLType::get(LST_STRING), "foobar")
    );
    symbol_table->define(allocator->new_tracked<LSLSymbol>(
        "whatever", LSLType::get(LST_INTEGER), SYM_FUNCTION, SYM_BUILTIN, func_dec
    ));
    FancyAddReplacementVisitor visitor;
    script->visit(&visitor);
    // all existing constant values are now potentially dirty, recalculate.
    script->propagate_values();
  });
}

TEST_SUITE_END();
