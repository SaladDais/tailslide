#include "tree_print.hh"
#include "lslmini.hh"

namespace Sling {


bool TreePrintingVisitor::visit(LLASTNode* node) {
  LLASTNode *child = node->get_children();
  char buf[256];

  auto lloc = node->get_lloc();
  auto type = node->get_type();
  auto constant_value = node->get_constant_value();
  int i;
  for (i = 0; i < walklevel; i++) {
    stream << "  ";
  }
  stream << node->get_node_name()
         << " [" << (type ? type->get_node_name() : "") << "] "
         << "(cv=" << (constant_value ? constant_value->get_node_name() : "") << ") "
         << '(' << lloc->first_line << ',' << lloc->first_column << ")\n";

  ++walklevel;
  visit_children(node);
  --walklevel;
  return false;
}

}
