#include <string>

#include "lslmini.hh"
#include "allocator.hh"
#include "operations.hh"


#define RET_IF_ZERO(_x) if(!(_x)) return NULL

namespace Tailslide {

LLScriptConstant *TailslideOperationBehavior::operation(
    int oper, LLScriptConstant *cv, LLScriptConstant *other_cv, YYLTYPE *lloc) {

  auto *left_type = cv->get_type();

  switch (left_type->get_itype()) {
    case LST_STRING:
      return operation(oper, (LLScriptStringConstant*)cv, other_cv, lloc);
    case LST_INTEGER:
      return operation(oper, (LLScriptIntegerConstant*)cv, other_cv, lloc);
    case LST_FLOATINGPOINT:
      return operation(oper, (LLScriptFloatConstant*)cv, other_cv, lloc);
    case LST_LIST:
      return operation(oper, (LLScriptListConstant*)cv, other_cv, lloc);
    case LST_VECTOR:
      return operation(oper, (LLScriptVectorConstant*)cv, other_cv, lloc);
    case LST_QUATERNION:
      return operation(oper, (LLScriptQuaternionConstant*)cv, other_cv, lloc);
    case LST_MAX:
    case LST_NULL:
    case LST_KEY:
    case LST_ERROR:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Integer Constants
LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptIntegerConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  S32 value = cv->get_value();
  // unary op
  if (other_const == nullptr) {
    int nv;
    switch (operation) {
      case '!':
        nv = !value;
        break;
      case '~':
        nv = ~value;
        break;
      case '-':
        nv = -value;
        break;
      default:
        return nullptr;
    }
    return gAllocationManager->new_tracked<LLScriptIntegerConstant>(nv);
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LLScriptIntegerConstant *) other_const)->get_value();
      int nv;
      switch (operation) {
        case '+':
          nv = value + ov;
          break;
        case '-':
          nv = value - ov;
          break;
        case '*':
          nv = value * ov;
          break;
        case '/':
          RET_IF_ZERO(ov);
          // Protect against underflows, see SL-31252
          if (ov == -1)
            nv = -1 * ov;
          else
            nv = value / ov;
          break;
        case '%': {
          RET_IF_ZERO(ov);
          // Protect against underflows, see SL-31252
          if (ov == -1 || ov == 1)
            nv = 0;
          else
            nv = value % ov;
          break;
        }
        case '&':
          nv = value & ov;
          break;
        case '|':
          nv = value | ov;
          break;
        case '^':
          nv = value ^ ov;
          break;
        case '>':
          nv = value > ov;
          break;
        case '<':
          nv = value < ov;
          break;
        case SHIFT_LEFT:
          nv = value << ov;
          break;
        case SHIFT_RIGHT:
          nv = value >> ov;
          break;
        case BOOLEAN_AND:
          nv = value && ov;
          break;
        case BOOLEAN_OR:
          nv = value || ov;
          break;
        case EQ:
          nv = value == ov;
          break;
        case NEQ:
          nv = value != ov;
          break;
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptIntegerConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LLScriptFloatConstant *) other_const)->get_value();
      float nv;
      switch (operation) {
        case '+':
          nv = (float)value + ov;
          break;
        case '-':
          nv = (float)value - ov;
          break;
        case '*':
          nv = (float)value * ov;
          break;
        case '/':
          // No division by zero plz
          RET_IF_ZERO(ov);
          nv = (float)value / ov;
          break;
        case '>':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>((float)value > ov);
        case '<':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>((float)value < ov);
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>((float)value == ov);
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>((float)value != ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptFloatConstant>(nv);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Float Constants
LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptFloatConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  float value = cv->get_value();
  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LLScriptFloatConstant>(-value);
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LLScriptIntegerConstant *) other_const)->get_value();
      float nv;
      switch (operation) {
        case '+':
          nv = value + (float)ov;
          break;
        case '-':
          nv = value - (float)ov;
          break;
        case '*':
          nv = value * (float)ov;
          break;
        case '/':
          RET_IF_ZERO(ov);
          nv = value / (float)ov;
          break;
        case '>':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value > (float)ov);
        case '<':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value < (float)ov);
        case GEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value >= (float)ov);
        case LEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value <= (float)ov);
        case BOOLEAN_AND:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value != 0.0 && ov);
        case BOOLEAN_OR:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value != 0.0 || ov);
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value == (float)ov);
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value != (float)ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptFloatConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LLScriptFloatConstant *) other_const)->get_value();
      float nv;
      switch (operation) {
        case '+':
          nv = value + ov;
          break;
        case '-':
          nv = value - ov;
          break;
        case '*':
          nv = value * ov;
          break;
        case '/':
          RET_IF_ZERO(ov);
          nv = value / ov;
          break;
        case '>':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value > ov);
        case '<':
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value < ov);
        case GEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value >= ov);
        case LEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value <= ov);
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value == ov);
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(value != ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptFloatConstant>(nv);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// String Constants
inline char *join_string(char *left, char *right) {
  char *ns = gAllocationManager->alloc(strlen(left) + strlen(right) + 1);
  strcpy(ns, left);
  strcat(ns, right);
  return ns;
}

LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptStringConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  char *value = cv->get_value();
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_STRING_CONSTANT: {
      char *ov = ((LLScriptStringConstant *) other_const)->get_value();
      switch (operation) {
        case '+':
          return gAllocationManager->new_tracked<LLScriptStringConstant>(join_string(value, ov));
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(!strcmp(value, ov));
          // If you want LSO's behaviour, remove the `!= 0`.
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(strcmp(value, ov) != 0);
        default:
          return nullptr;
      }
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// List Constants
LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptListConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_LIST_CONSTANT: {
      LLScriptListConstant *other = ((LLScriptListConstant *) other_const);
      switch (operation) {
        case EQ:
          // warn on list == list
          if (cv->get_length() != 0 && other->get_length() != 0) {
            ERROR(lloc, W_LIST_COMPARE);
          }
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(
            cv->get_length() == other->get_length()
          );
        case NEQ:
          // warn on list == list
          if (cv->get_length() != 0 && other->get_length() != 0) {
            ERROR(lloc, W_LIST_COMPARE);
          }
          // Yes, really.
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(
            cv->get_length() - other->get_length()
          );
        default:
          return nullptr;
      }
    }
    default:
      return nullptr;
  }
}


//////////////////////////////////////////////
// Vector Constants
LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptVectorConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  LLVector *value = cv->get_value();
  // Make sure we have a value
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LLScriptVectorConstant>(-value->x, -value->y, -value->z);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LLScriptIntegerConstant *) other_const)->get_value();
      float nv[3];
      switch (operation) {
        case '*':
          nv[0] = value->x * (float)ov;
          nv[1] = value->y * (float)ov;
          nv[2] = value->z * (float)ov;
          break;
        case '/':
          RET_IF_ZERO(ov);
          nv[0] = value->x / (float)ov;
          nv[1] = value->y / (float)ov;
          nv[2] = value->z / (float)ov;
          break;
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LLScriptFloatConstant *) other_const)->get_value();
      float nv[3];
      switch (operation) {
        case '*':
          nv[0] = value->x * ov;
          nv[1] = value->y * ov;
          nv[2] = value->z * ov;
          break;
        case '/':
          RET_IF_ZERO(ov);
          nv[0] = value->x / ov;
          nv[1] = value->y / ov;
          nv[2] = value->z / ov;
          break;
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_VECTOR_CONSTANT: {
      LLVector *ov = ((LLScriptVectorConstant *) other_const)->get_value();
      if (ov == nullptr)
        return nullptr;
      float nv[3];
      switch (operation) {
        case '+':
          nv[0] = value->x + ov->x;
          nv[1] = value->y + ov->y;
          nv[2] = value->z + ov->z;
          break;
        case '-':
          nv[0] = value->x - ov->x;
          nv[1] = value->y - ov->y;
          nv[2] = value->z - ov->z;
          break;
        case '*':
          return gAllocationManager->new_tracked<LLScriptFloatConstant>((value->x * ov->z) + (value->y * ov->y) + (value->z * ov->x));
        case '%':           // cross product
          nv[0] = (value->y * ov->z) - (value->z * ov->y);
          nv[1] = (value->z * ov->x) - (value->x * ov->z);
          nv[2] = (value->x * ov->y) - (value->y * ov->x);
          break;
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(*value == *ov);
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(*value != *ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LLScriptVectorConstant>(nv[0], nv[1], nv[2]);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Quaternion Constants
LLScriptConstant *TailslideOperationBehavior::operation(
    int operation, LLScriptQuaternionConstant *cv, LLScriptConstant *other_const, YYLTYPE *lloc) {
  LLQuaternion *value = cv->get_value();
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LLScriptQuaternionConstant>(-value->x, -value->y, -value->z, -value->s);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_QUATERNION_CONSTANT: {
      LLQuaternion *ov = ((LLScriptQuaternionConstant *) other_const)->get_value();
      if (ov == nullptr)
        return nullptr;
      switch (operation) {
        case EQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(*value == *ov);
        case NEQ:
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(*value != *ov);
        case '-':
          return gAllocationManager->new_tracked<LLScriptQuaternionConstant>(value->x - ov->x, value->y - ov->y, value->z - ov->z, value->s - ov->s);
        default:
          return nullptr;
      }
    }
    default:
      return nullptr;
  }
}



//////
// Casts
//////

LLScriptConstant *TailslideOperationBehavior::cast(LLScriptType *to_type, LLScriptConstant *cv, YYLTYPE *lloc) {

  auto *orig_type = cv->get_type();
  if (orig_type == to_type) {
    // no-op case
    return cv;
  }

  switch (orig_type->get_itype()) {
    case LST_STRING:
      return cast(to_type, (LLScriptStringConstant *)cv, lloc);
    case LST_INTEGER:
      return cast(to_type, (LLScriptIntegerConstant *)cv, lloc);
    case LST_FLOATINGPOINT:
      return cast(to_type, (LLScriptFloatConstant *)cv, lloc);
    case LST_LIST:
      return cast(to_type, (LLScriptListConstant *)cv, lloc);
    case LST_VECTOR:
      return cast(to_type, (LLScriptVectorConstant *)cv, lloc);
    case LST_QUATERNION:
      return cast(to_type, (LLScriptQuaternionConstant *)cv, lloc);
    case LST_MAX:
    case LST_NULL:
    case LST_KEY:
    case LST_ERROR:
      return nullptr;
  }
}



LLScriptConstant* TailslideOperationBehavior::cast(LLScriptType *to_type, LLScriptStringConstant *cv, YYLTYPE *lloc) {
  auto *v = cv->get_value();
  switch(to_type->get_itype()) {
    case LST_INTEGER: {
      int base = 10;
      // Need to explicitly determine what the base should be, we only support
      // base 10 and base 16 and we don't want `011` to be treated as octal!
      // This check is safe because `cv` must be a null terminated string.
      if (v[0] == '0' && (v[1] == 'x' || v[2] == 'X'))
        base = 16;
      return gAllocationManager->new_tracked<LLScriptIntegerConstant>((S32)strtoul(v, nullptr, base));
    }
    case LST_FLOATINGPOINT: {
      return gAllocationManager->new_tracked<LLScriptFloatConstant>((F32)strtod(v, nullptr));
    }
    default:
      return nullptr;
  }
}

LLScriptConstant* TailslideOperationBehavior::cast(LLScriptType *to_type, LLScriptIntegerConstant *cv, YYLTYPE *lloc) {
  auto v = cv->get_value();
  switch(to_type->get_itype()) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LLScriptStringConstant>(
          gAllocationManager->copy_str(std::to_string(v).c_str())
      );
    }
    default:
      return nullptr;
  }
}

LLScriptConstant* TailslideOperationBehavior::cast(LLScriptType *to_type, LLScriptFloatConstant *cv, YYLTYPE *lloc) {
  auto v = cv->get_value();
  switch(to_type->get_itype()) {
    case LST_STRING: {
      std::string f_as_str {std::to_string(v)};
      if (f_as_str == "inf")
        f_as_str = "Infinity";
      else if (f_as_str == "-inf")
        f_as_str = "-Infinity";
        // Only one kind of NaN in LSL!
      else if (f_as_str == "nan" || f_as_str == "-nan")
        f_as_str = "NaN";

      return gAllocationManager->new_tracked<LLScriptStringConstant>(
          gAllocationManager->copy_str(f_as_str.c_str())
      );
    }
    default:
      return nullptr;
  }
}

}