#include <iomanip>
#include <cmath>

#include "pretty_print.hh"
#include "lslmini.hh"
#include "strings.hh"


namespace Tailslide {

class ScopedTabSetter {
public:
  ScopedTabSetter(PrettyPrintVisitor *visitor, int tabs): visitor(visitor), old_tabs(visitor->tabs) {
    visitor->tabs = tabs;
  };
  ~ScopedTabSetter() {
    visitor->tabs = old_tabs;
  }
private:
  int old_tabs;
  PrettyPrintVisitor *visitor;
};

static bool cast_requires_parens(LSLTypecastExpression* node) {
  switch (node->get_child(0)->get_node_type()) {
    case NODE_CONSTANT:
    case NODE_IDENTIFIER:
      return false;
    default:;
  }

  switch (node->get_child(0)->get_node_sub_type()) {
    case NODE_PRINT_EXPRESSION:
    case NODE_LVALUE_EXPRESSION:
    case NODE_FUNCTION_EXPRESSION:
    case NODE_LIST_EXPRESSION:
    case NODE_QUATERNION_EXPRESSION:
    case NODE_VECTOR_EXPRESSION:
    case NODE_CONSTANT_EXPRESSION:
      return false;
    default:;
  }
  return true;
}

// Check if this if/for/while/etc. body has braces
static int is_braceless(LSLASTNode *node) {
  return (node->get_node_sub_type() != NODE_COMPOUND_STATEMENT);
}

void PrettyPrintVisitor::prettify_siblings_sep(LSLASTNode* node, const char* separator) {
  while ( node != nullptr ) {
    node->visit(this);
    node = node->get_next();
    if (node)
      stream << separator;
  }
}

// special-case for vectors and quaternions so we don't accidentally output
// expressions in the members in a context where the brackets could be interpreted
// as shift operators
void PrettyPrintVisitor::prettify_coordinate_members(LSLASTNode *node) {
  std::stringstream orig_stream(std::move(stream));
  stream = std::stringstream();
  prettify_siblings_sep(node, ", ");
  std::string innards {stream.str()};
  stream = std::move(orig_stream);
  if (innards.front() == '<')
    stream << ' ';
  stream << innards;
  if (innards.back() == '>')
    stream << ' ';
}

bool PrettyPrintVisitor::visit(LSLASTNode *node) {
  std::stringstream descriptor_sstr;
  descriptor_sstr << "UNKNOWN NODE \"" << node->get_node_name() << "\" @ " << (void *)this << "> */";
  std::string node_descriptor = descriptor_sstr.str();

  do_tabs();
  stream << "/* <" << node_descriptor << "\n";
  visit_children(node);
  do_tabs();
  stream << "/* </" << node_descriptor << "\n";

  return false;
}

bool PrettyPrintVisitor::visit(LSLIdentifier *node) {
  LSLSymbol *sym = node->get_symbol();
  const char *name = node->get_name();
  if (sym != nullptr && sym->get_mangled_name()
      && ((opts.mangle_local_names && sym->get_sub_type() == SYM_LOCAL)
          || (opts.mangle_global_names && sym->get_sub_type() != SYM_LOCAL && sym->get_symbol_type() != SYM_FUNCTION)
          || (opts.mangle_func_names && sym->get_symbol_type() == SYM_FUNCTION))) {
    if (opts.show_unmangled) {
      stream << "/*" << name << "*/";
    }
    stream << sym->get_mangled_name();
  } else {
    stream << name;
  }
  return false;
}




//
// Global Stuff
//

bool PrettyPrintVisitor::visit(LSLState *node) {
  do_tabs();
  LSLASTNode *state_id = node->get_child(0);
  if (state_id->get_node_type() == NODE_NULL)
    stream << "default";
  else {
    stream << "state ";
    state_id->visit(this);
  }

  stream << "\n{\n";

  // Ick. Seems like these should all be under one node.
  // All children from this point on are event handlers.
  {
    ScopedTabSetter setter(this, tabs + 1);
    prettify_siblings_sep(node->get_child(1), "\n");
  }
  do_tabs();
  stream << "}\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLEventHandler *node) {
  do_tabs();

  // event name
  node->get_child(0)->visit(this);
  // params
  stream << '(';
  node->get_child(1)->visit(this);
  stream << ")\n";
  // body
  node->get_child(2)->visit(this);
  return false;
}

bool PrettyPrintVisitor::visit(LSLGlobalVariable *node) {
  do_tabs();

  LSLASTNode* id = node->get_child(0);
  id->get_type()->visit(this);
  stream << ' ';
  id->visit(this);

  LSLASTNode* assignable = node->get_child(1);
  if (assignable->get_node_type() != NODE_NULL) {
    stream << " = ";
    assignable->visit(this);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLGlobalFunction *node) {
  do_tabs();
  // print the return type if we have one
  if (node->get_child(0)->get_type()->get_itype() != LST_NULL) {
    node->get_child(0)->get_type()->visit(this);
    stream << ' ';
  }

  node->get_child(0)->visit(this);
  stream << '(';
  node->get_child(1)->visit(this);
  stream << ")\n";
  node->get_child(2)->visit(this);
  stream << '\n';
  return false;
}

bool PrettyPrintVisitor::visit(LSLParamList *node) {
  LSLASTNode *child_node = node->get_children();
  while (child_node && child_node->get_node_type() != NODE_NULL ) {
    child_node->get_type()->visit(this);
    stream << ' ';
    child_node->visit(this);
    child_node = child_node->get_next();
    if (child_node)
      stream << ", ";
  }
  return false;
}





//
// Statements
//

bool PrettyPrintVisitor::visit(LSLStatement *node) {
  do_tabs();
  // Not sure about the separator, but looks like that's what lscript does?
  prettify_siblings_sep(node->get_children(), ", ");
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLCompoundStatement *node) {
  do_tabs();
  stream << "{\n";

  {
    ScopedTabSetter setter(this, tabs + 1);
    visit_children(node);
  }

  do_tabs();
  stream << "}\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLDeclaration *node) {
  do_tabs();

  LSLASTNode* id = node->get_child(0);
  id->get_type()->visit(this);
  stream << ' ';
  id->visit(this);

  LSLASTNode* assignable = node->get_child(1);
  if (assignable->get_node_type() != NODE_NULL) {
    stream << " = ";
    assignable->visit(this);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLStateStatement *node) {
  do_tabs();
  stream << "state ";
  if (node->get_child(0) == nullptr || node->get_child(0)->get_node_type() == NODE_NULL)
    stream << "default";
  else
    visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLLabel *node) {
  do_tabs();
  stream << '@';
  visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLJumpStatement *node) {
  do_tabs();
  stream << "jump ";
  visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLReturnStatement *node) {
  do_tabs();
  stream << "return";
  if (node->get_child(0) != nullptr && node->get_child(0)->get_node_type() != NODE_NULL) {
    stream << ' ';
    visit_children(node);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLIfStatement *node) {
  LSLASTNode* expr = node->get_child(0);
  LSLASTNode* true_branch = node->get_child(1);
  LSLASTNode* false_branch = node->get_child(2);

  // Don't indent the "if" in "else if", but _do_ invent if we're in the true branch.
  // Basically, perverse things like:
  // if (foo)
  //     if (bar)
  //         baz();
  LSLASTNode * parent = node->get_parent();
  if (parent->get_node_sub_type() != NODE_IF_STATEMENT || parent->get_child(1) == node)
    do_tabs();

  stream << "if (";
  expr->visit(this);
  stream << ")\n";
  // Only increase indentation of the child if this is a braceless `if` branch
  {
    ScopedTabSetter setter(this, tabs + is_braceless(true_branch));
    true_branch->visit(this);
  }

  if (false_branch->get_node_type() != NODE_NULL) {
    do_tabs();
    stream << "else";
    bool chained_if = false_branch->get_node_sub_type() == NODE_IF_STATEMENT;
    if (chained_if)
      stream << ' ';
    else
      stream << '\n';

    {
      ScopedTabSetter setter(this, tabs + (is_braceless(false_branch) && !chained_if));
      false_branch->visit(this);
    }
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLWhileStatement *node) {
  do_tabs();
  stream << "while (";
  node->get_child(0)->visit(this);
  stream << ")\n";
  LSLASTNode* body = node->get_child(1);
  {
    ScopedTabSetter setter(this, tabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLDoStatement *node) {
  do_tabs();
  stream << "do\n";
  LSLASTNode* body = node->get_child(0);
  {
    ScopedTabSetter setter(this, tabs + is_braceless(body));
    body->visit(this);
  }
  do_tabs();
  stream << "while(";
  node->get_child(1)->visit(this);
  stream << ");\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLForStatement *node) {
  do_tabs();
  stream << "for (";
  node->get_child(0)->visit(this);
  stream << "; ";
  node->get_child(1)->visit(this);
  stream << "; ";
  node->get_child(2)->visit(this);
  stream << ")\n";

  LSLASTNode *body = node->get_child(3);
  {
    ScopedTabSetter setter(this, tabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLForExpressionList *node) {
  prettify_siblings_sep(node->get_children(), ", ");
  return false;
}



//
// Expressions
//

bool PrettyPrintVisitor::visit(LSLExpression *node) {
  LSLASTNode *left = node->get_child(0);
  LSLASTNode *right = node->get_child(1);
  int operation = node->get_operation();

  if (!operation) {
    left->visit(this);
  } else if (right) {
    left->visit(this);
    stream << ' ' << operation_str(operation) << ' ';
    right->visit(this);
  } else {
    if (operation == INC_POST_OP || operation == DEC_POST_OP) {
      left->visit(this);
      stream << operation_str(operation);
    } else if (operation == '(') {
      stream << '(';
      left->visit(this);
      stream << ')';

    // Make sure we don't end up with `--2147483648` on underflow...
    } else if (operation == '-') {
      stream << '-';
      bool need_parens = false;
      if (left->get_child(0)->get_node_sub_type() == NODE_INTEGER_CONSTANT) {
        need_parens = ((LSLIntegerConstant*)left->get_child(0))->get_value() < 0;
      }
      if (need_parens)
        stream << '(';
      left->visit(this);
      if (need_parens)
        stream << ')';
    } else {
      stream << operation_str(operation);
      left->visit(this);
    }
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLLValueExpression *node) {
  node->get_child(0)->visit(this);
  LSLASTNode *member = node->get_child(1);
  if (member && member->get_node_type() != NODE_NULL) {
    stream << '.';
    member->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLFunctionExpression *node) {
  node->get_child(0)->visit(this);
  stream << '(';
  prettify_siblings_sep(node->get_child(1), ", ");
  stream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLTypecastExpression *node) {
  stream << '(';
  node->get_type()->visit(this);
  stream << ')';
  bool need_parens = cast_requires_parens(node);
  if (need_parens)
    stream << '(';
  visit_children(node);
  if (need_parens)
    stream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLVectorExpression *node) {
  stream << '<';
  prettify_coordinate_members(node->get_children());
  stream << '>';
  return false;
}

bool PrettyPrintVisitor::visit(LSLQuaternionExpression *node) {
  stream << '<';
  prettify_coordinate_members(node->get_children());
  stream << '>';
  return false;
}

static std::string format_float(float v) {
  if (!std::isfinite(v)) {
    // this really should never happen, as LSL doesn't have a NaN literal and we should
    // refuse to fold constants that result in NaN, but as a last resort try typecasting
    // from string (which will not be legal in a global context.)
    if (std::isnan(v)) {
      return "((string)\"nan\")";
    } else if (v > 0.0) {
      // about as close as we can get to an inf literal in LSL, exponent places it outside
      // the range of both float and double width.
      return "2.0e+999";
    } else {
      return "-2.0e+999";
    }
  }
  char pretty_buf[256];
  snprintf(pretty_buf, 256, "%#.6g", v);
  return pretty_buf;
}

bool PrettyPrintVisitor::visit(LSLVectorConstant *node) {
  LSLVector *v = node->get_value();
  assert (v != nullptr);
  stream << '<';
  stream << format_float(v->x);
  stream << ", ";
  stream << format_float(v->y);
  stream << ", ";
  stream << format_float(v->z);
  stream << ">";
  return false;
}

bool PrettyPrintVisitor::visit(LSLQuaternionConstant *node) {
  LSLQuaternion *v = node->get_value();
  assert (v != nullptr);
  stream << '<';
  stream << format_float(v->x);
  stream << ", ";
  stream << format_float(v->y);
  stream << ", ";
  stream << format_float(v->z);
  stream << ", ";
  stream << format_float(v->s);
  stream << ">";
  return false;
}

bool PrettyPrintVisitor::visit(LSLListExpression *node) {
  stream << '[';
  prettify_siblings_sep(node->get_children(), ", ");
  stream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LSLListConstant *node) {
  stream << '[';
  prettify_siblings_sep(node->get_value(), ", ");
  stream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LSLIntegerConstant *node) {
  stream << node->get_value();
  return false;
}

bool PrettyPrintVisitor::visit(LSLFloatConstant *node) {
  stream << format_float(node->get_value());
  return false;
}

bool PrettyPrintVisitor::visit(LSLStringConstant *node) {
  stream << '"' << escape_string(node->get_value()) << '"';
  return false;
}

bool PrettyPrintVisitor::visit(LSLPrintExpression *node) {
  stream << "print(";
  visit_children(node);
  stream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLType* node) {
  if (node->get_itype() != LST_NULL) {
    stream << node->get_node_name();
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLGlobalStorage *node) {
  return true;
}

bool PrettyPrintVisitor::visit(LSLScript *node) {
  return true;
}

void PrettyPrintVisitor::do_tabs() {
  for(int i=0; i<tabs; ++i) {
    stream << "    ";
  }
}

}
