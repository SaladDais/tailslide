#ifndef TAILSLIDE_TREE_PRINT_HH
#define TAILSLIDE_TREE_PRINT_HH

#include <sstream>

#include "visitor.hh"

namespace Tailslide {

class TreePrintingVisitor: public ASTVisitor {
  public:
    virtual bool visit(LLASTNode *node);
    int walklevel = 0;
    std::stringstream stream {};
};
}


#endif //TAILSLIDE_TREE_PRINT_HH
