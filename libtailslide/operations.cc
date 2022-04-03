#include <string>

#include "lslmini.hh"
#include "allocator.hh"
#include "operations.hh"


#define RET_IF_ZERO(_x) if(!(_x)) return NULL

namespace Tailslide {

LSLConstant *TailslideOperationBehavior::operation(
    int oper, LSLConstant *cv, LSLConstant *other_cv, YYLTYPE *lloc) {

  auto *left_type = cv->get_type();
  LSLConstant *new_cv = nullptr;

  switch (left_type->get_itype()) {
    case LST_STRING:
      new_cv = operation(oper, (LSLStringConstant*)cv, other_cv);
      break;
    case LST_INTEGER:
      new_cv = operation(oper, (LSLIntegerConstant*)cv, other_cv);
      break;
    case LST_FLOATINGPOINT:
      new_cv = operation(oper, (LSLFloatConstant*)cv, other_cv);
      break;
    case LST_LIST:
      new_cv = operation(oper, (LSLListConstant*)cv, other_cv);
      break;
    case LST_VECTOR:
      new_cv = operation(oper, (LSLVectorConstant*)cv, other_cv);
      break;
    case LST_QUATERNION:
      new_cv = operation(oper, (LSLQuaternionConstant*)cv, other_cv);
      break;
    case LST_MAX:
    case LST_NULL:
    case LST_KEY:
    case LST_ERROR:
      return nullptr;
  }
  if (new_cv)
    new_cv->set_lloc(lloc);
  return new_cv;
}

//////////////////////////////////////////////
// Integer Constants
LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLIntegerConstant *cv, LSLConstant *other_const) {
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
    return gAllocationManager->new_tracked<LSLIntegerConstant>(nv);
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->get_value();
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
      return gAllocationManager->new_tracked<LSLIntegerConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->get_value();
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
          return gAllocationManager->new_tracked<LSLIntegerConstant>((float)value > ov);
        case '<':
          return gAllocationManager->new_tracked<LSLIntegerConstant>((float)value < ov);
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>((float)value == ov);
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>((float)value != ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LSLFloatConstant>(nv);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Float Constants
LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLFloatConstant *cv, LSLConstant *other_const) {
  float value = cv->get_value();
  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LSLFloatConstant>(-value);
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->get_value();
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
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value > (float)ov);
        case '<':
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value < (float)ov);
        case GEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value >= (float)ov);
        case LEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value <= (float)ov);
        case BOOLEAN_AND:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value != 0.0 && ov);
        case BOOLEAN_OR:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value != 0.0 || ov);
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value == (float)ov);
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value != (float)ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LSLFloatConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->get_value();
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
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value > ov);
        case '<':
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value < ov);
        case GEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value >= ov);
        case LEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value <= ov);
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value == ov);
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(value != ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LSLFloatConstant>(nv);
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

LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLStringConstant *cv, LSLConstant *other_const) {
  char *value = cv->get_value();
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_STRING_CONSTANT: {
      char *ov = ((LSLStringConstant *) other_const)->get_value();
      switch (operation) {
        case '+':
          return gAllocationManager->new_tracked<LSLStringConstant>(join_string(value, ov));
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(!strcmp(value, ov));
          // If you want LSO's behaviour, remove the `!= 0`.
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(strcmp(value, ov) != 0);
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
LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLListConstant *cv, LSLConstant *other_const) {
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_LIST_CONSTANT: {
      LSLListConstant *other = ((LSLListConstant *) other_const);
      switch (operation) {
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(
            cv->get_length() == other->get_length()
          );
        case NEQ:
          // Yes, really.
          return gAllocationManager->new_tracked<LSLIntegerConstant>(
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
LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLVectorConstant *cv, LSLConstant *other_const) {
  Vector3 *value = cv->get_value();
  // Make sure we have a value
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LSLVectorConstant>(-value->x, -value->y, -value->z);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->get_value();
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
      return gAllocationManager->new_tracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->get_value();
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
      return gAllocationManager->new_tracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_VECTOR_CONSTANT: {
      Vector3 *ov = ((LSLVectorConstant *) other_const)->get_value();
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
          return gAllocationManager->new_tracked<LSLFloatConstant>((value->x * ov->z) + (value->y * ov->y) + (value->z * ov->x));
        case '%':           // cross product
          nv[0] = (value->y * ov->z) - (value->z * ov->y);
          nv[1] = (value->z * ov->x) - (value->x * ov->z);
          nv[2] = (value->x * ov->y) - (value->y * ov->x);
          break;
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(*value == *ov);
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(*value != *ov);
        default:
          return nullptr;
      }
      return gAllocationManager->new_tracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Quaternion Constants
LSLConstant *TailslideOperationBehavior::operation(
    int operation, LSLQuaternionConstant *cv, LSLConstant *other_const) {
  Quaternion *value = cv->get_value();
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return gAllocationManager->new_tracked<LSLQuaternionConstant>(-value->x, -value->y, -value->z, -value->s);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->get_node_sub_type()) {
    case NODE_QUATERNION_CONSTANT: {
      Quaternion *ov = ((LSLQuaternionConstant *) other_const)->get_value();
      if (ov == nullptr)
        return nullptr;
      switch (operation) {
        case EQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(*value == *ov);
        case NEQ:
          return gAllocationManager->new_tracked<LSLIntegerConstant>(*value != *ov);
        case '-':
          return gAllocationManager->new_tracked<LSLQuaternionConstant>(value->x - ov->x, value->y - ov->y, value->z - ov->z, value->s - ov->s);
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

LSLConstant *TailslideOperationBehavior::cast(LSLType *to_type, LSLConstant *cv, YYLTYPE *lloc) {

  auto *orig_type = cv->get_type();
  if (orig_type == to_type) {
    // no-op case
    return cv;
  }

  LSLConstant *new_cv = nullptr;
  switch (orig_type->get_itype()) {
    case LST_STRING:
      new_cv = cast(to_type, (LSLStringConstant *)cv);
      break;
    case LST_INTEGER:
      new_cv = cast(to_type, (LSLIntegerConstant *)cv);
      break;
    case LST_FLOATINGPOINT:
      new_cv = cast(to_type, (LSLFloatConstant *)cv);
      break;
    case LST_LIST:
      new_cv = cast(to_type, (LSLListConstant *)cv);
      break;
    case LST_VECTOR:
      new_cv = cast(to_type, (LSLVectorConstant *)cv);
      break;
    case LST_QUATERNION:
      new_cv = cast(to_type, (LSLQuaternionConstant *)cv);
      break;
    case LST_MAX:
    case LST_NULL:
    case LST_KEY:
    case LST_ERROR:
      return nullptr;
  }
  if (new_cv)
    new_cv->set_lloc(lloc);
  return new_cv;
}



LSLConstant* TailslideOperationBehavior::cast(LSLType *to_type, LSLStringConstant *cv) {
  auto *v = cv->get_value();
  switch(to_type->get_itype()) {
    case LST_INTEGER: {
      int base = 10;
      // Need to explicitly determine what the base should be, we only support
      // base 10 and base 16 and we don't want `011` to be treated as octal!
      // This check is safe because `cv` must be a null terminated string.
      if (v[0] == '0' && (v[1] == 'x' || v[2] == 'X'))
        base = 16;
      return gAllocationManager->new_tracked<LSLIntegerConstant>((S32)strtoul(v, nullptr, base));
    }
    case LST_FLOATINGPOINT: {
      return gAllocationManager->new_tracked<LSLFloatConstant>((F32)strtod(v, nullptr));
    }
    default:
      return nullptr;
  }
}

LSLConstant* TailslideOperationBehavior::cast(LSLType *to_type, LSLIntegerConstant *cv) {
  auto v = cv->get_value();
  switch(to_type->get_itype()) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LSLStringConstant>(
          gAllocationManager->copy_str(std::to_string(v).c_str())
      );
    }
    default:
      return nullptr;
  }
}

LSLConstant* TailslideOperationBehavior::cast(LSLType *to_type, LSLFloatConstant *cv) {
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

      return gAllocationManager->new_tracked<LSLStringConstant>(
          gAllocationManager->copy_str(f_as_str.c_str())
      );
    }
    default:
      return nullptr;
  }
}

}