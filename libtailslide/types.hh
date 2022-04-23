#ifndef _TYPES_HH
#define _TYPES_HH 1
#include "ast.hh"           // LSLASTNode
#include <vector>

namespace Tailslide {

class LSLType : public LSLASTNode {
  public:
    explicit LSLType(LSLIType type, bool static_def=false) : LSLASTNode(nullptr), _mIType(type) {
      // Parenting the global LSLType instances to a specific script's tree is illegal
      if (static_def)
        markStatic();
    };
    static LSLType *get(LSLIType type ) {
      return &_sTypes[type];
    }
    bool canCoerce(LSLType *to );
    LSLType *getResultType(int op, LSLType *right);


    LSLIType getIType() { return _mIType; };
    virtual const char *getNodeName() {
      switch (_mIType) {
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
    virtual LSLNodeType getNodeType() { return NODE_TYPE; };

    class LSLConstant *getDefaultValue() { return _mDefaultVal; }
    void setDefaultValue(class LSLConstant *default_val) { _mDefaultVal = default_val; }
    class LSLConstant *getOneValue() { return _mOneVal; }
    void setOneValue(class LSLConstant *one_val) { _mOneVal = one_val; }
  private:
    LSLIType _mIType;
    class LSLConstant *_mDefaultVal = nullptr;
    class LSLConstant *_mOneVal = nullptr;
    static LSLType _sTypes[LST_MAX];
};

// convenience
inline LSLType *TYPE(LSLIType type) {
  return LSLType::get(type);
}

int decouple_compound_operation(int operation);
bool operation_mutates(int operation);
const char* operation_str(int operation);
const char* operation_repr_str(int operation);
bool is_cast_legal(LSLIType from, LSLIType to);
}

#endif /* not _TYPES_HH */
