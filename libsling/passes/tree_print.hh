#ifndef SLING_TREE_PRINT_HH
#define SLING_TREE_PRINT_HH

#include <sstream>

#include "visitor.hh"

namespace Sling {

class TreePrintingVisitor: public ASTVisitor {
  public:
    virtual bool visit(LLASTNode *node);
    int walklevel = 0;
    std::stringstream stream {};
};
}


#endif //SLING_TREE_PRINT_HH
