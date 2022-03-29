#include <string>

#include "lslmini.hh"
#include "allocator.hh"

#define RET_IF_ZERO(_x) if(!(_x)) return NULL

namespace Tailslide {

//////////////////////////////////////////////
// Integer Constants
LLScriptConstant *LLScriptIntegerConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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
LLScriptConstant *LLScriptFloatConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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

LLScriptConstant *LLScriptStringConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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
LLScriptConstant *LLScriptListConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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
          if (get_length() != 0 && other->get_length() != 0) {
            ERROR(lloc, W_LIST_COMPARE);
          }
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(get_length() == other->get_length());
        case NEQ:
          // warn on list == list
          if (get_length() != 0 && other->get_length() != 0) {
            ERROR(lloc, W_LIST_COMPARE);
          }
          // Yes, really.
          return gAllocationManager->new_tracked<LLScriptIntegerConstant>(get_length() - other->get_length());
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
LLScriptConstant *LLScriptVectorConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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
LLScriptConstant *LLScriptQuaternionConstant::operation(int operation, LLScriptConstant *other_const, YYLTYPE *lloc) {
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


LLScriptConstant* LLScriptStringConstant::cast(LST_TYPE to_type) {
  auto *cv = ((LLScriptStringConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_INTEGER: {
      int base = 10;
      // Need to explicitly determine what the base should be, we only support
      // base 10 and base16 and we don't want `011` to be treated as octal!
      // This check is safe because `cv` must be a null terminated string.
      if (cv[0] == '0' && (cv[1] == 'x' || cv[2] == 'X'))
        base = 16;
      return gAllocationManager->new_tracked<LLScriptIntegerConstant>((S32)strtoul(cv, nullptr, base));
    }
    case LST_FLOATINGPOINT: {
      return gAllocationManager->new_tracked<LLScriptFloatConstant>((F32)strtod(cv, nullptr));
    }
    default:
      return nullptr;
  }
}

LLScriptConstant* LLScriptIntegerConstant::cast(LST_TYPE to_type) {
  auto cv = ((LLScriptIntegerConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LLScriptStringConstant>(gAllocationManager->copy_str(
          std::to_string(cv).c_str()
      ));
    }
    default:
      return nullptr;
  }
}

LLScriptConstant *LLScriptFloatConstant::cast(LST_TYPE to_type) {
  auto cv = ((LLScriptFloatConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LLScriptStringConstant>(gAllocationManager->copy_str(
          // to_string correctly handles -inf and friends
          std::to_string(cv).c_str()
      ));
    }
    default:
      return nullptr;
  }
}

}
