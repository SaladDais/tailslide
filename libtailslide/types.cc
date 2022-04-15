#include "lslmini.hh"
#include "types.hh"

#define  LST_ANY        -1
#define  LST_NONE       -2
#define  LST_BOOLEAN    LST_INTEGER     // for clarity

namespace Tailslide {
// TODO: use structs or something here

const static int COERCION_TABLE[][2] = {
        // wanted type      acceptable type
        {LST_FLOATINGPOINT, LST_INTEGER},
        {LST_STRING,        LST_KEY},
        {LST_KEY,           LST_STRING},
        {-1,                -1},
};

// from->to allowed lookup table
const static int LEGAL_CAST_TABLE[LST_MAX][LST_MAX] = {
    /* NULL   */ {0},
    /* INT    */ {0, 1, 1, 1, 0, 0, 0, 1, 0},
    /* FLOAT  */ {0, 1, 1, 1, 0, 0, 0, 1, 0},
    /* STRING */ {0, 1, 1, 1, 1, 1, 1, 1, 0},
    /* KEY    */ {0, 0, 0, 1, 1, 0, 0, 1, 0},
    /* VECTOR */ {0, 0, 0, 1, 0, 1, 0, 1, 0},
    /* QUAT   */ {0, 0, 0, 1, 0, 0, 1, 1, 0},
    /* LIST   */ {0, 0, 0, 1, 0, 0, 0, 1, 0},
    /* ERROR  */ {0},
};

const static int OPERATOR_RESULTS[][4] = {

        // operator   left type           right type          result type
        // ++
        {INC_PRE_OP,  LST_INTEGER,       LST_NONE,          LST_INTEGER},
        {INC_PRE_OP,  LST_FLOATINGPOINT, LST_NONE,          LST_FLOATINGPOINT},
        {INC_POST_OP, LST_INTEGER,       LST_NONE,          LST_INTEGER},
        {INC_POST_OP, LST_FLOATINGPOINT, LST_NONE,          LST_FLOATINGPOINT},

        // --
        {DEC_PRE_OP,  LST_INTEGER,       LST_NONE,          LST_INTEGER},
        {DEC_PRE_OP,  LST_FLOATINGPOINT, LST_NONE,          LST_FLOATINGPOINT},
        {DEC_POST_OP, LST_INTEGER,       LST_NONE,          LST_INTEGER},
        {DEC_POST_OP, LST_FLOATINGPOINT, LST_NONE,          LST_FLOATINGPOINT},

        // =
        {'=',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'=',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'=',         LST_FLOATINGPOINT, LST_INTEGER,       LST_FLOATINGPOINT},
        {'=',         LST_STRING,        LST_STRING,        LST_STRING},
        {'=',         LST_STRING,        LST_KEY,           LST_STRING},
        {'=',         LST_KEY,           LST_STRING,        LST_KEY},
        {'=',         LST_KEY,           LST_KEY,           LST_KEY},
        {'=',         LST_VECTOR,        LST_VECTOR,        LST_VECTOR},
        {'=',         LST_QUATERNION,    LST_QUATERNION,    LST_QUATERNION},
        {'=',         LST_LIST,          LST_LIST,          LST_LIST},

        // -
        {'-',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'-',         LST_INTEGER,       LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'-',         LST_FLOATINGPOINT, LST_INTEGER,       LST_FLOATINGPOINT},
        {'-',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'-',         LST_VECTOR,        LST_VECTOR,        LST_VECTOR},
        {'-',         LST_QUATERNION,    LST_QUATERNION,    LST_QUATERNION},

        // unary -
        {'-',         LST_INTEGER,       LST_NONE,          LST_INTEGER},
        {'-',         LST_FLOATINGPOINT, LST_NONE,          LST_FLOATINGPOINT},
        {'-',         LST_VECTOR,        LST_NONE,          LST_VECTOR},
        {'-',         LST_QUATERNION,    LST_NONE,          LST_QUATERNION},

        // +
        {'+',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'+',         LST_INTEGER,       LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'+',         LST_FLOATINGPOINT, LST_INTEGER,       LST_FLOATINGPOINT},
        {'+',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'+',         LST_STRING,        LST_STRING,        LST_STRING},
        {'+',         LST_VECTOR,        LST_VECTOR,        LST_VECTOR},
        {'+',         LST_QUATERNION,    LST_QUATERNION,    LST_QUATERNION},
        {'+',         LST_LIST,          LST_ANY,           LST_LIST},
        {'+',         LST_ANY,           LST_LIST,          LST_LIST},

        // *
        {'*',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'*',         LST_INTEGER,       LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'*',         LST_INTEGER,       LST_VECTOR,        LST_VECTOR}, // scale vector
        {'*',         LST_FLOATINGPOINT, LST_INTEGER,       LST_FLOATINGPOINT},
        {'*',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'*',         LST_FLOATINGPOINT, LST_VECTOR,        LST_VECTOR}, // scale vector
        {'*',         LST_VECTOR,        LST_INTEGER,       LST_VECTOR}, // scale vector
        {'*',         LST_VECTOR,        LST_FLOATINGPOINT, LST_VECTOR}, // scale vector
        {'*',         LST_VECTOR,        LST_VECTOR,        LST_FLOATINGPOINT}, // dot product
        {'*',         LST_VECTOR,        LST_QUATERNION,    LST_VECTOR}, // rotate
        {'*',         LST_QUATERNION,    LST_QUATERNION,    LST_QUATERNION}, // merge rotations, effectively

        // /
        {'/',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'/',         LST_INTEGER,       LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'/',         LST_FLOATINGPOINT, LST_INTEGER,       LST_FLOATINGPOINT},
        {'/',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {'/',         LST_VECTOR,        LST_INTEGER,       LST_VECTOR}, // scale vector
        {'/',         LST_VECTOR,        LST_FLOATINGPOINT, LST_VECTOR}, // scale vector
        {'/',         LST_VECTOR,        LST_QUATERNION,    LST_VECTOR}, // rotate by inverse of rside
        {'/',         LST_QUATERNION,    LST_QUATERNION,    LST_QUATERNION}, // rotate by inverse of rside

        // %
        {'%',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'%',         LST_VECTOR,        LST_VECTOR,        LST_VECTOR}, // cross product

        // >
        {'>',         LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {'>',         LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {'>',         LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {'>',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},

        // <
        {'<',         LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {'<',         LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {'<',         LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {'<',         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},

        // <=
        {LEQ,         LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {LEQ,         LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {LEQ,         LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {LEQ,         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},

        // >=
        {GEQ,         LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {GEQ,         LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {GEQ,         LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {GEQ,         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},

        // ==
        {EQ,          LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {EQ,          LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {EQ,          LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {EQ,          LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},
        {EQ,          LST_VECTOR,        LST_VECTOR,        LST_BOOLEAN},
        {EQ,          LST_QUATERNION,    LST_QUATERNION,    LST_BOOLEAN},
        {EQ,          LST_STRING,        LST_STRING,        LST_BOOLEAN},
        {EQ,          LST_STRING,        LST_KEY,           LST_BOOLEAN},
        {EQ,          LST_KEY,           LST_STRING,        LST_BOOLEAN},
        {EQ,          LST_KEY,           LST_KEY,           LST_BOOLEAN},
        {EQ,          LST_LIST,          LST_LIST,          LST_BOOLEAN},

        // !=
        {NEQ,         LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {NEQ,         LST_INTEGER,       LST_FLOATINGPOINT, LST_BOOLEAN},
        {NEQ,         LST_FLOATINGPOINT, LST_INTEGER,       LST_BOOLEAN},
        {NEQ,         LST_FLOATINGPOINT, LST_FLOATINGPOINT, LST_BOOLEAN},
        {NEQ,         LST_VECTOR,        LST_VECTOR,        LST_BOOLEAN},
        {NEQ,         LST_QUATERNION,    LST_QUATERNION,    LST_BOOLEAN},
        // This seriously returns an int, see lscript_execute:list_list_operation()
        // and lsa_cmp_lists. NEQ returns `(len(a) - len(b))`.
        // `(list1 != list2)` is not the same as `!(list1 == list2)`.
        {NEQ,         LST_STRING,        LST_STRING,        LST_INTEGER},
        {NEQ,         LST_STRING,        LST_KEY,           LST_INTEGER},
        {NEQ,         LST_KEY,           LST_STRING,        LST_INTEGER},
        {NEQ,         LST_KEY,           LST_KEY,           LST_INTEGER},
        // Similar to above, but the same under LSO2 and Mono
        {NEQ,         LST_LIST,          LST_LIST,          LST_INTEGER},

        // bitwise operators
        {'&',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'|',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'^',         LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {'~',         LST_INTEGER,       LST_NONE,          LST_INTEGER},

        // boolean opeartors
        {'!',         LST_INTEGER,       LST_NONE,          LST_BOOLEAN},
        {BOOLEAN_AND, LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},
        {BOOLEAN_OR,  LST_INTEGER,       LST_INTEGER,       LST_BOOLEAN},

        // shift operators
        {SHIFT_LEFT,  LST_INTEGER,       LST_INTEGER,       LST_INTEGER},
        {SHIFT_RIGHT, LST_INTEGER,       LST_INTEGER,       LST_INTEGER},

        // end
        {-1,          -1,                -1,                -1},
};

LSLType LSLType::_sTypes[LST_MAX] = { // NOLINT(cert-err58-cpp)
        LSLType(LST_NULL, true),
        LSLType(LST_INTEGER, true),
        LSLType(LST_FLOATINGPOINT, true),
        LSLType(LST_STRING, true),
        LSLType(LST_KEY, true),
        LSLType(LST_VECTOR, true),
        LSLType(LST_QUATERNION, true),
        LSLType(LST_LIST, true),
        LSLType(LST_ERROR, true),
};

bool LSLType::canCoerce(LSLType *to) {
  int i;

  // error type matches anything
  if (getIType() == LST_ERROR || to->getIType() == LST_ERROR)
    return true;

  // if we're already of the target type, then of course we can be used for it
  if (getIType() == to->getIType())
    return true;

  for (i = 0; COERCION_TABLE[i][1] != -1; i++) {
    if (COERCION_TABLE[i][1] == getIType() && COERCION_TABLE[i][0] == to->getIType()) {
      return true;
    }
  }
  return false;
}

class LSLType *LSLType::getResultType(int op, LSLType *right) {
  int i;

  // error on either side is always error
  if (getIType() == LST_ERROR || (right != nullptr && right->getIType() == LST_ERROR))
    return TYPE(LST_ERROR);

  if (op == '(') {
    return right;
  }

  // *_ASSIGN is usually just syntactic sugar. `foo *= 3` is the same as `foo = foo * 3`
  bool compound_assignment = true;
  switch (op) {
    case ADD_ASSIGN: op = '+'; break;
    case SUB_ASSIGN: op = '-'; break;
    case MUL_ASSIGN: op = '*'; break;
    case DIV_ASSIGN: op = '/'; break;
    case MOD_ASSIGN: op = '%'; break;
    default:
      compound_assignment = false;
      break;
  }

  // go through each entry in the operator result table
  for (i = 0; OPERATOR_RESULTS[i][0] != -1; i++) {

    // no operator match
    if (OPERATOR_RESULTS[i][0] != op)
      continue;

    // the left side must match our left side
    if (OPERATOR_RESULTS[i][1] != getIType() && OPERATOR_RESULTS[i][1] != LST_ANY)
      continue;

    bool match;

    if (right == nullptr)
      // right IS empty and matches nothing
      match = (OPERATOR_RESULTS[i][2] == LST_NONE);
    else
      // or right isn't empty and matches our side
      match = (OPERATOR_RESULTS[i][2] == LST_ANY ||
               OPERATOR_RESULTS[i][2] == (int) right->getIType());

    // no type match on the operation
    if (!match)
      continue;

    // send back the type
    auto *ret_type = TYPE((LSLIType) OPERATOR_RESULTS[i][3]);
    // for compound assignment operators the type of the operation's retval
    // must additionally match the type of the lvalue, or it is not a valid
    // compound assignment.
    // For example, `int_val += 1.0` and `vec *= <1,1,1>` are forbidden.
    // but something like `float_val += 1` is fine.
    if (compound_assignment && ret_type != this) {
      // ... is mostly true, but not entirely. There's one case in LL's compiler
      // (that was probably a mistake) where `int_val *= float_val` is allowed.
      // `int_val = int_val * float_val` is not legal since `int_val` must be promoted
      // to a float which can't be assigned to an int lvalue, but the compound form doesn't
      // even behave as you'd expect.
      // In LSO it behaves the same as `(int_val = (integer)(int_val * float_val)) * 0.0`.
      // In Mono it causes a runtime VM error due to invalid IL if you actually try to use
      // the retval in something like `llOwnerSay((string)(int_val *= float_val))`.
      // For now let's just warn and pretend it returns a float, because it sort of does in LSO.
      if (getIType() == LST_INTEGER && right && right->getIType() == LST_FLOATINGPOINT && op == '*') {
        return TYPE(LST_FLOATINGPOINT);
      }
      return nullptr;
    }
    return ret_type;
  }
  return nullptr;
}

bool operation_mutates(int operation) {
  switch (operation) {
    case '=':
    case INC_PRE_OP:
    case INC_POST_OP:
    case DEC_PRE_OP:
    case DEC_POST_OP:
    case ADD_ASSIGN:
    case SUB_ASSIGN:
    case MUL_ASSIGN:
    case DIV_ASSIGN:
    case MOD_ASSIGN:
      return true;
    default:
      return false;
  }
}

const char *operation_str(int operation) {
  switch (operation) {
    case 0:
      return "<constant>";
    case '=':
      return "=";
    case NEQ:
      return "!=";
    case EQ:
      return "==";
    case GEQ:
      return ">=";
    case LEQ:
      return "<=";
    case INC_PRE_OP:
      return "++";
    case DEC_PRE_OP:
      return "--";
    case INC_POST_OP:
      return "++";
    case DEC_POST_OP:
      return "--";
    case BOOLEAN_AND:
      return "&&";
    case BOOLEAN_OR:
      return "||";
    case SHIFT_LEFT:
      return "<<";
    case SHIFT_RIGHT:
      return ">>";
    case ADD_ASSIGN:
      return "+=";
    case SUB_ASSIGN:
      return "-=";
    case MUL_ASSIGN:
      return "*=";
    case DIV_ASSIGN:
      return "/=";
    case MOD_ASSIGN:
      return "%=";
    case '+':
      return "+";
    case '-':
      return "-";
    case '*':
      return "*";
    case '/':
      return "/";
    case '%':
      return "%";
    case '~':
      return "~";
    case '^':
      return "^";
    case '&':
      return "&";
    case '|':
      return "|";
    case '!':
      return "!";
    case '<':
      return "<";
    case '>':
      return ">";
    case '(':
      return "()";
    default:
      return "<BUSTED>";
  }
}

const char *operation_repr_str(int operation) {
  switch(operation) {
    case INC_POST_OP:
      return "++ (post)";
    case INC_PRE_OP:
      return "++ (pre)";
    case DEC_POST_OP:
      return "-- (post)";
    case DEC_PRE_OP:
      return "-- (pre)";
    default:
      return operation_str(operation);
  }
}

bool is_cast_legal(LSLIType from, LSLIType to) {
  return LEGAL_CAST_TABLE[from][to] == 1;
}
}
