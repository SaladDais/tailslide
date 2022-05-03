#include <iomanip>
#include <cmath>

#include "pretty_print.hh"
#include "../lslmini.hh"
#include "../strings.hh"


namespace Tailslide {

class ScopedTabSetter {
public:
  ScopedTabSetter(PrettyPrintVisitor *visitor, int tabs): _mVisitor(visitor), _mOldTabs(visitor->mTabs) {
    _mVisitor->mTabs = tabs;
  };
  ~ScopedTabSetter() {
    _mVisitor->mTabs = _mOldTabs;
  }
private:
  int _mOldTabs;
  PrettyPrintVisitor *_mVisitor;
};

static bool cast_requires_parens(LSLTypecastExpression* node) {
  auto *child_expr = node->getChildExpr();
  switch (child_expr->getNodeType()) {
    case NODE_CONSTANT:
    case NODE_IDENTIFIER:
      return false;
    default:;
  }

  switch (child_expr->getNodeSubType()) {
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
  return (node->getNodeSubType() != NODE_COMPOUND_STATEMENT);
}

void PrettyPrintVisitor::prettifyChildrenSep(LSLASTNode* parent, const char* separator) {
  for (auto *child: *parent) {
    child->visit(this);
    if (child->getNext())
      mStream << separator;
  }
}

// special-case for vectors and quaternions so we don't accidentally output
// expressions in the members in a context where the brackets could be interpreted
// as shift operators
void PrettyPrintVisitor::prettifyCoordinateMembers(LSLASTNode *node) {
  std::stringstream orig_stream(std::move(mStream));
  mStream = std::stringstream();
  prettifyChildrenSep(node, ", ");
  std::string innards {mStream.str()};
  mStream = std::move(orig_stream);
  if (innards.front() == '<')
    mStream << ' ';
  mStream << innards;
  if (innards.back() == '>')
    mStream << ' ';
}

bool PrettyPrintVisitor::visit(LSLASTNode *node) {
  std::stringstream descriptor_sstr;
  descriptor_sstr << "UNKNOWN NODE \"" << node->getNodeName() << "\" @ " << (void *)this << "> */";
  std::string node_descriptor = descriptor_sstr.str();

  doTabs();
  mStream << "/* <" << node_descriptor << "\n";
  visitChildren(node);
  doTabs();
  mStream << "/* </" << node_descriptor << "\n";

  return false;
}

bool PrettyPrintVisitor::visit(LSLIdentifier *node) {
  LSLSymbol *sym = node->getSymbol();
  const char *name = node->getName();
  if (sym != nullptr && sym->getMangledName()
      && ((_mOpts.mangle_local_names && sym->getSubType() == SYM_LOCAL)
          || (_mOpts.mangle_global_names && sym->getSubType() != SYM_LOCAL && sym->getSymbolType() != SYM_FUNCTION)
          || (_mOpts.mangle_func_names && sym->getSymbolType() == SYM_FUNCTION))) {
    if (_mOpts.show_unmangled) {
      mStream << "/*" << name << "*/";
    }
    mStream << sym->getMangledName();
  } else {
    mStream << name;
  }
  return false;
}




//
// Global Stuff
//

bool PrettyPrintVisitor::visit(LSLState *node) {
  doTabs();
  auto *state_id = node->getIdentifier();
  if (!strcmp("default", state_id->getName()))
    mStream << "default";
  else {
    mStream << "state ";
    state_id->visit(this);
  }

  mStream << "\n{\n";

  // Print all of the event handlers under this state
  {
    ScopedTabSetter setter(this, mTabs + 1);
    prettifyChildrenSep(node->getEventHandlers(), "\n");
  }
  doTabs();
  mStream << "}\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLEventHandler *node) {
  doTabs();

  // event name
  node->getIdentifier()->visit(this);
  // params
  mStream << '(';
  node->getArguments()->visit(this);
  mStream << ")\n";
  // body
  node->getStatements()->visit(this);
  return false;
}

bool PrettyPrintVisitor::visit(LSLGlobalVariable *node) {
  doTabs();

  LSLASTNode* id = node->getIdentifier();
  id->getType()->visit(this);
  mStream << ' ';
  id->visit(this);

  if (auto *assignable = node->getInitializer()) {
    mStream << " = ";
    assignable->visit(this);
  }
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLGlobalFunction *node) {
  doTabs();
  // print the return type if we have one
  auto *id = node->getIdentifier();
  if (id->getIType() != LST_NULL) {
    id->getType()->visit(this);
    mStream << ' ';
  }

  id->visit(this);
  mStream << '(';
  node->getArguments()->visit(this);
  mStream << ")\n";
  node->getStatements()->visit(this);
  mStream << '\n';
  return false;
}

bool PrettyPrintVisitor::visit(LSLParamList *node) {
  for (auto *child_node : *node) {
    child_node->getType()->visit(this);
    mStream << ' ';
    child_node->visit(this);
    if (child_node->getNext())
      mStream << ", ";
  }
  return false;
}





//
// Statements
//

bool PrettyPrintVisitor::visit(LSLStatement *node) {
  doTabs();
  // handles NopStatements and ExpressionStatements.
  if (auto *child = node->getChild(0))
    child->visit(this);
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLCompoundStatement *node) {
  doTabs();
  mStream << "{\n";

  {
    ScopedTabSetter setter(this, mTabs + 1);
    visitChildren(node);
  }

  doTabs();
  mStream << "}\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLDeclaration *node) {
  doTabs();

  LSLASTNode* id = node->getIdentifier();
  id->getType()->visit(this);
  mStream << ' ';
  id->visit(this);

  if (auto *assignable = node->getInitializer()) {
    mStream << " = ";
    assignable->visit(this);
  }
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLStateStatement *node) {
  doTabs();
  mStream << "state ";
  node->getIdentifier()->visit(this);
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLLabel *node) {
  doTabs();
  mStream << '@';
  node->getIdentifier()->visit(this);
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLJumpStatement *node) {
  doTabs();
  mStream << "jump ";
  node->getIdentifier()->visit(this);
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLReturnStatement *node) {
  doTabs();
  mStream << "return";
  if (auto *ret_expr = node->getExpr()) {
    mStream << ' ';
    ret_expr->visit(this);
  }
  mStream << ";\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLIfStatement *node) {
  // Don't indent the "if" in "else if", but _do_ invent if we're in the true branch.
  // Basically, perverse things like:
  // if (foo)
  //     if (bar)
  //         baz();
  LSLASTNode * parent = node->getParent();
  if (parent->getNodeSubType() != NODE_IF_STATEMENT || parent->getChild(1) == node)
    doTabs();

  mStream << "if (";
  node->getCheckExpr()->visit(this);
  mStream << ")\n";
  // Only increase indentation of the child if this is a braceless `if` branch
  {
    LSLASTNode* true_branch = node->getTrueBranch();
    ScopedTabSetter setter(this, mTabs + is_braceless(true_branch));
    true_branch->visit(this);
  }

  if (auto *false_branch=node->getFalseBranch()) {
    doTabs();
    mStream << "else";
    bool chained_if = false_branch->getNodeSubType() == NODE_IF_STATEMENT;
    if (chained_if)
      mStream << ' ';
    else
      mStream << '\n';

    {
      ScopedTabSetter setter(this, mTabs + (is_braceless(false_branch) && !chained_if));
      false_branch->visit(this);
    }
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLWhileStatement *node) {
  doTabs();
  mStream << "while (";
  node->getCheckExpr()->visit(this);
  mStream << ")\n";
  {
    auto *body = node->getBody();
    ScopedTabSetter setter(this, mTabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLDoStatement *node) {
  doTabs();
  mStream << "do\n";
  {
    auto *body = node->getBody();
    ScopedTabSetter setter(this, mTabs + is_braceless(body));
    body->visit(this);
  }
  doTabs();
  mStream << "while(";
  node->getCheckExpr()->visit(this);
  mStream << ");\n";
  return false;
}

bool PrettyPrintVisitor::visit(LSLForStatement *node) {
  doTabs();
  mStream << "for (";
  prettifyChildrenSep(node->getInitExprs(), ", ");
  mStream << "; ";
  node->getCheckExpr()->visit(this);
  mStream << "; ";
  prettifyChildrenSep(node->getIncrExprs(), ", ");
  mStream << ")\n";

  {
    auto *body = node->getBody();
    ScopedTabSetter setter(this, mTabs + is_braceless(body));
    body->visit(this);
  }
  return false;
}



//
// Expressions
//

bool PrettyPrintVisitor::visit(LSLParenthesisExpression *node) {
  mStream << '(';
  node->getChildExpr()->visit(this);
  mStream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLUnaryExpression *node) {
  auto *expr = node->getChildExpr();
  LSLOperator operation = node->getOperation();
  if (operation == OP_POST_INCR || operation == OP_POST_DECR) {
    expr->visit(this);
    mStream << operation_str(operation);
  } else if (operation == '-') {
    // Make sure we don't end up with `--2147483648` on underflow...
    bool need_parens = false;
    if (expr->getChild(0)->getNodeSubType() == NODE_INTEGER_CONSTANT) {
      need_parens = ((LSLIntegerConstant *) expr->getChild(0))->getValue() < 0;
    }
    mStream << '-';
    if (need_parens)
      mStream << '(';
    expr->visit(this);
    if (need_parens)
      mStream << ')';
  } else {
    mStream << operation_str(operation);
    expr->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLBinaryExpression *node) {
  node->getLHS()->visit(this);
  mStream << ' ' << operation_str(node->getOperation()) << ' ';
  node->getRHS()->visit(this);
  return false;
}

bool PrettyPrintVisitor::visit(LSLLValueExpression *node) {
  node->getIdentifier()->visit(this);
  if (auto *member = node->getMember()) {
    mStream << '.';
    member->visit(this);
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLConstantExpression *node) {
  node->getConstantValue()->visit(this);
  return false;
}

bool PrettyPrintVisitor::visit(LSLFunctionExpression *node) {
  node->getIdentifier()->visit(this);
  mStream << '(';
  prettifyChildrenSep(node->getArguments(), ", ");
  mStream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLTypecastExpression *node) {
  mStream << '(';
  node->getType()->visit(this);
  mStream << ')';
  bool need_parens = cast_requires_parens(node);
  if (need_parens)
    mStream << '(';
  node->getChildExpr()->visit(this);
  if (need_parens)
    mStream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLVectorExpression *node) {
  mStream << '<';
  prettifyCoordinateMembers(node);
  mStream << '>';
  return false;
}

bool PrettyPrintVisitor::visit(LSLQuaternionExpression *node) {
  mStream << '<';
  prettifyCoordinateMembers(node);
  mStream << '>';
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
      // oddly enough these _are_ allowed as initializers for globals under LSO, because the fault
      // register gets cleared at the end of compilation. The value just can't ever be used.
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
  const Vector3 *v = node->getValue();
  assert (v != nullptr);
  mStream << '<';
  mStream << format_float(v->x);
  mStream << ", ";
  mStream << format_float(v->y);
  mStream << ", ";
  mStream << format_float(v->z);
  mStream << ">";
  return false;
}

bool PrettyPrintVisitor::visit(LSLQuaternionConstant *node) {
  const Quaternion *v = node->getValue();
  assert (v != nullptr);
  mStream << '<';
  mStream << format_float(v->x);
  mStream << ", ";
  mStream << format_float(v->y);
  mStream << ", ";
  mStream << format_float(v->z);
  mStream << ", ";
  mStream << format_float(v->s);
  mStream << ">";
  return false;
}

bool PrettyPrintVisitor::visit(LSLListExpression *node) {
  mStream << '[';
  prettifyChildrenSep(node, ", ");
  mStream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LSLListConstant *node) {
  mStream << '[';
  prettifyChildrenSep(node, ", ");
  mStream << ']';
  return false;
}

bool PrettyPrintVisitor::visit(LSLIntegerConstant *node) {
  mStream << node->getValue();
  return false;
}

bool PrettyPrintVisitor::visit(LSLFloatConstant *node) {
  mStream << format_float(node->getValue());
  return false;
}

bool PrettyPrintVisitor::visit(LSLStringConstant *node) {
  mStream << '"' << escape_string(node->getValue()) << '"';
  return false;
}

bool PrettyPrintVisitor::visit(LSLKeyConstant *node) {
  return visit((LSLStringConstant *)node);
}

bool PrettyPrintVisitor::visit(LSLPrintExpression *node) {
  mStream << "print(";
  visitChildren(node);
  mStream << ')';
  return false;
}

bool PrettyPrintVisitor::visit(LSLType* node) {
  if (node->getIType() != LST_NULL) {
    mStream << node->getNodeName();
  }
  return false;
}

bool PrettyPrintVisitor::visit(LSLScript *node) {
  return true;
}

void PrettyPrintVisitor::doTabs() {
  for(int i=0; i < mTabs; ++i) {
    mStream << "    ";
  }
}

}
