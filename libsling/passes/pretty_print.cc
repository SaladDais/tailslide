#include <iomanip>

#include "pretty_print.hh"
#include "lslmini.hh"


namespace Sling {

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

static bool cast_requires_parens(LLScriptTypecastExpression* node) {
  switch (node->get_child(0)->get_node_type()) {
    case NODE_CONSTANT:
    case NODE_SIMPLE_ASSIGNABLE:
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
static int is_braceless(LLASTNode *node) {
  return (node->get_node_sub_type() != NODE_COMPOUND_STATEMENT);
}

void PrettyPrintVisitor::prettify_siblings_sep(LLASTNode* node, const char* separator) {
  while ( node != nullptr ) {
    node->visit(this);
    node = node->get_next();
    if (node)
      stream << separator;
  }
}

bool PrettyPrintVisitor::visit(LLASTNode *node) {
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

bool PrettyPrintVisitor::visit(LLScriptIdentifier *node) {
  LLScriptSymbol *sym = node->get_symbol();
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
  const char *member = node->get_member();
  if (member) {
    stream << '.' << member;
  }
  return false;
}




//
// Global Stuff
//

bool PrettyPrintVisitor::visit(LLScriptState *node) {
  do_tabs();
  LLASTNode *state_id = node->get_child(0);
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

bool PrettyPrintVisitor::visit(LLScriptEventHandler *node) {
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

bool PrettyPrintVisitor::visit(LLScriptGlobalVariable *node) {
  do_tabs();

  LLASTNode* id = node->get_child(0);
  id->get_type()->visit(this);
  stream << ' ';
  id->visit(this);

  LLASTNode* assignable = node->get_child(1);
  if (assignable->get_node_type() != NODE_NULL) {
    stream << " = ";
    assignable->visit(this);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptGlobalFunction *node) {
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

bool PrettyPrintVisitor::visit(LLScriptParamList *node) {
  LLASTNode *child_node = node->get_children();
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

bool PrettyPrintVisitor::visit(LLScriptStatement *node) {
  do_tabs();
  // Not sure about the separator, but looks like that's what lscript does?
  prettify_siblings_sep(node->get_children(), ", ");
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptCompoundStatement *node) {
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

bool PrettyPrintVisitor::visit(LLScriptDeclaration *node) {
  do_tabs();

  LLASTNode* id = node->get_child(0);
  id->get_type()->visit(this);
  stream << ' ';
  id->visit(this);

  LLASTNode* assignable = node->get_child(1);
  if (assignable->get_node_type() != NODE_NULL) {
    stream << " = ";
    assignable->visit(this);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptStateStatement *node) {
  do_tabs();
  stream << "state ";
  if (node->get_child(0) == nullptr || node->get_child(0)->get_node_type() == NODE_NULL)
    stream << "default";
  else
    visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptLabel *node) {
  do_tabs();
  stream << '@';
  visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptJumpStatement *node) {
  do_tabs();
  stream << "jump ";
  visit_children(node);
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptReturnStatement *node) {
  do_tabs();
  stream << "return";
  if (node->get_child(0) != nullptr && node->get_child(0)->get_node_type() != NODE_NULL) {
    stream << ' ';
    visit_children(node);
  }
  stream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptIfStatement *node) {
  LLASTNode* expr = node->get_child(0);
  LLASTNode* true_branch = node->get_child(1);
  LLASTNode* false_branch = node->get_child(2);

  // Don't indent the "if" in "else if", but _do_ invent if we're in the true branch.
  // Basically, perverse things like:
  // if (foo)
  //     if (bar)
  //         baz();
  LLASTNode * parent = node->get_parent();
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

bool PrettyPrintVisitor::visit(LLScriptWhileStatement *node) {
  do_tabs();
  stream << "while (";
  node->get_child(0)->visit(this);
  stream << ")\n";
  LLASTNode* body = node->get_child(1);
  {
    ScopedTabSetter setter(this, tabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptDoStatement *node) {
  do_tabs();
  stream << "do\n";
  LLASTNode* body = node->get_child(0);
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

bool PrettyPrintVisitor::visit(LLScriptForStatement *node) {
  do_tabs();
  stream << "for (";
  node->get_child(0)->visit(this);
  stream << "; ";
  node->get_child(1)->visit(this);
  stream << "; ";
  node->get_child(2)->visit(this);
  stream << ")\n";

  LLASTNode *body = node->get_child(3);
  {
    ScopedTabSetter setter(this, tabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptForExpressionList *node) {
  prettify_siblings_sep(node->get_children(), ", ");
  return false;
}



//
// Expressions
//

bool PrettyPrintVisitor::visit(LLScriptExpression *node) {
  LLASTNode *left = node->get_child(0);
  LLASTNode *right = node->get_child(1);
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
        need_parens = ((LLScriptIntegerConstant*)left->get_child(0))->get_value() < 0;
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

bool PrettyPrintVisitor::visit(LLScriptFunctionExpression *node) {
  node->get_child(0)->visit(this);
  stream << '(';
  prettify_siblings_sep(node->get_child(1), ", ");
  stream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptTypecastExpression *node) {
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

bool PrettyPrintVisitor::visit(LLScriptVectorExpression *node) {
  stream << '<';
  prettify_siblings_sep(node->get_children(), ", ");
  stream << '>';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptQuaternionExpression *node) {
  stream << '<';
  prettify_siblings_sep(node->get_children(), ", ");
  stream << '>';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptVectorConstant *node) {
  LLVector *v = node->get_value();
  if (v != nullptr) {
    char pretty_buf[256];
    snprintf(pretty_buf, 256, "<%#.6g, %#.6g, %#.6g>", v->x, v->y, v->z);
    stream << pretty_buf;
  } else {
    stream << '<';
    prettify_siblings_sep(node->get_children(), ", ");
    stream << '>';
  }
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptQuaternionConstant *node) {
  LLQuaternion *v = node->get_value();
  if (v != nullptr) {
    char pretty_buf[256];
    snprintf(pretty_buf, 256, "<%#.6g, %#.6g, %#.6g, %#.6g>", v->x, v->y, v->z, v->s);
    stream << pretty_buf;
  } else {
    stream << '<';
    prettify_siblings_sep(node->get_children(), ", ");
    stream << '>';
  }
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptListExpression *node) {
  stream << '[';
  prettify_siblings_sep(node->get_children(), ", ");
  stream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptListConstant *node) {
  stream << '[';
  prettify_siblings_sep(node->get_value(), ", ");
  stream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptIntegerConstant *node) {
  stream << node->get_value();
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptFloatConstant *node) {
  char pretty_buf[256];
  snprintf(pretty_buf, 256, "%#.6g", node->get_value());
  stream << pretty_buf;
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptStringConstant *node) {
  stream << '"' << escape_str(node->get_value()) << '"';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptPrintExpression *node) {
  stream << "print(";
  visit_children(node);
  stream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptType* node) {
  if (node->get_itype() != LST_NULL) {
    stream << node->get_node_name();
  }
  return false;
}

bool PrettyPrintVisitor::visit(LLScriptGlobalStorage *node) {
  return true;
}

bool PrettyPrintVisitor::visit(LLScriptScript *node) {
  return true;
}

bool PrettyPrintVisitor::visit(LLScriptSimpleAssignable *node) {
  return true;
}

void PrettyPrintVisitor::do_tabs() {
  for(int i=0; i<tabs; ++i) {
    stream << "    ";
  }
}

std::string PrettyPrintVisitor::escape_str(const char *data) {
  std::string new_str;
  size_t datasize = strlen(data);
  for(int i=0; i<datasize; ++i) {
    char c = data[i];
    if (c == '\\')
      new_str += "\\\\";
    else if (c == '"')
      new_str += "\\\"";
    else if (c == '\n')
      new_str += "\\n";
    else
      new_str += c;
  }
  return new_str;
}
}
