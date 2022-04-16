#ifndef _TYPES_HH
#define _TYPES_HH 1
#include "ast.hh"           // LSLASTNode
#include <vector>

namespace Tailslide {

class LSLType : public LSLASTNode {
  public:
    explicit LSLType(LST_TYPE type, bool static_def=false) : LSLASTNode(nullptr), itype(type) {
      // Parenting the global LSLType instances to a specific script's tree is illegal
      if (static_def)
        mark_static();
    };
    static LSLType *get( LST_TYPE type ) {
      return &types[type];
    }
    bool can_coerce( LSLType *to );
    LSLType *get_result_type(int op,  LSLType *right);

    LST_TYPE get_itype() { return itype; } ;
    virtual const char *get_node_name() {
      switch (itype) {
        case LST_ERROR:         return "error";
        case LST_INTEGER:       return "integer";
        case LST_FLOATINGPOINT: return "float";
        case LST_STRING:        return "string";
        case LST_KEY:           return "key";
        case LST_VECTOR:        return "vector";
        case LST_QUATERNION:    return "quaternion";
        case LST_LIST:          return "list";
        case LST_NULL:          return "none";
        default:                return "!invalid!";
      }
    }
    virtual LSLNodeType get_node_type() { return NODE_TYPE; };
  private:
    LST_TYPE itype;
    static LSLType types[LST_MAX];
};

// convenience
inline LSLType *TYPE(LST_TYPE type) {
  return LSLType::get(type);
}

bool operation_mutates(int operation);
const char* operation_str(int operation);
const char* operation_repr_str(int operation);
bool is_cast_legal(LST_TYPE from, LST_TYPE to);
}

#endif /* not _TYPES_HH */
