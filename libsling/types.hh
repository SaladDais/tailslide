#ifndef _TYPES_HH
#define _TYPES_HH 1
#include "ast.hh"           // LLASTNode
#include <vector>

#define TYPE(t) LLScriptType::get(t)    // convenience

namespace Sling {
enum LST_TYPE {
  LST_ERROR         = 0,   // special value so processing can continue without throwing bogus errors
  LST_NULL          = 1,
  LST_INTEGER       = 2,
  LST_FLOATINGPOINT = 3,
  LST_STRING        = 4,
  LST_KEY           = 5,
  LST_VECTOR        = 6,
  LST_QUATERNION    = 7,
  LST_LIST          = 8,    // ??
  LST_MAX           = 9,
};

class LLScriptType : public LLASTNode {
  public:
    explicit LLScriptType(LST_TYPE type, bool static_def=false) : LLASTNode(0), itype(type) {
      // Parenting the global LLScriptType instances to a specific script's tree is illegal
      if (static_def)
        mark_static();
    };
    static LLScriptType *get( LST_TYPE type ) {
      return &types[type];
    }
    bool can_coerce( LLScriptType *to );
    LLScriptType *get_result_type(int op,  LLScriptType *right);

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
    virtual LLNodeType get_node_type() { return NODE_TYPE; };
  private:
    LST_TYPE itype;
    static LLScriptType types[LST_MAX];
};

bool operation_mutates(int operation);
const char* operation_str(int operation);
bool is_cast_legal(LST_TYPE from, LST_TYPE to);
}

#endif /* not _TYPES_HH */
