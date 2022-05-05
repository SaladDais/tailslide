#include <string>

#include "lslmini.hh"
#include "allocator.hh"
#include "operations.hh"


#define RET_IF_ZERO(_x) if(!(_x)) return NULL

namespace Tailslide {

LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator oper, LSLConstant *cv, LSLConstant *other_cv, YYLTYPE *lloc) {

  auto *left_type = cv->getType();
  LSLConstant *new_cv = nullptr;

  switch (left_type->getIType()) {
    case LST_STRING:
      new_cv = operation(oper, (LSLStringConstant*)cv, other_cv);
      break;
    case LST_KEY:
      new_cv = operation(oper, (LSLKeyConstant*)cv, other_cv);
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
    case LST_ERROR:
      return nullptr;
  }
  if (new_cv)
    new_cv->setLoc(lloc);
  return new_cv;
}

//////////////////////////////////////////////
// Integer Constants
LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator operation, LSLIntegerConstant *cv, LSLConstant *other_const) {
  S32 value = cv->getValue();
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
    return _mAllocator->newTracked<LSLIntegerConstant>(nv);
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->getValue();
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
        case OP_SHIFT_LEFT:
          nv = value << ov;
          break;
        case OP_SHIFT_RIGHT:
          nv = value >> ov;
          break;
        case OP_BOOLEAN_AND:
          nv = value && ov;
          break;
        case OP_BOOLEAN_OR:
          nv = value || ov;
          break;
        case OP_EQ:
          nv = value == ov;
          break;
        case OP_NEQ:
          nv = value != ov;
          break;
        default:
          return nullptr;
      }
      return _mAllocator->newTracked<LSLIntegerConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->getValue();
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
          return _mAllocator->newTracked<LSLIntegerConstant>((float) value > ov);
        case '<':
          return _mAllocator->newTracked<LSLIntegerConstant>((float) value < ov);
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>((float) value == ov);
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>((float) value != ov);
        default:
          return nullptr;
      }
      return _mAllocator->newTracked<LSLFloatConstant>(nv);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Float Constants
LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator operation, LSLFloatConstant *cv, LSLConstant *other_const) {
  float value = cv->getValue();
  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return _mAllocator->newTracked<LSLFloatConstant>(-value);
    return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->getValue();
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
          return _mAllocator->newTracked<LSLIntegerConstant>(value > (float) ov);
        case '<':
          return _mAllocator->newTracked<LSLIntegerConstant>(value < (float) ov);
        case OP_GEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value >= (float) ov);
        case OP_LEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value <= (float) ov);
        case OP_BOOLEAN_AND:
          return _mAllocator->newTracked<LSLIntegerConstant>(value != 0.0 && ov);
        case OP_BOOLEAN_OR:
          return _mAllocator->newTracked<LSLIntegerConstant>(value != 0.0 || ov);
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value == (float) ov);
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value != (float) ov);
        default:
          return nullptr;
      }
      return _mAllocator->newTracked<LSLFloatConstant>(nv);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->getValue();
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
          return _mAllocator->newTracked<LSLIntegerConstant>(value > ov);
        case '<':
          return _mAllocator->newTracked<LSLIntegerConstant>(value < ov);
        case OP_GEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value >= ov);
        case OP_LEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value <= ov);
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value == ov);
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(value != ov);
        default:
          return nullptr;
      }
      return _mAllocator->newTracked<LSLFloatConstant>(nv);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// String Constants

LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator operation, LSLStringConstant *cv, LSLConstant *other_const) {
  const char *value = cv->getValue();
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_STRING_CONSTANT: {
      const char *ov = ((LSLStringConstant *) other_const)->getValue();
      switch (operation) {
        case '+':
          if (_mMayCreateHeapValues) {
            return _mAllocator->newTracked<LSLStringConstant>(joinString(value, ov));
          } else {
            return nullptr;
          }
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(!strcmp(value, ov));
          // If you want LSO's behaviour, remove the `!= 0`.
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(strcmp(value, ov) != 0);
        default:
          return nullptr;
      }
    }
    case NODE_KEY_CONSTANT: {
      const char *ov = ((LSLStringConstant *) other_const)->getValue();
      switch (operation) {
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(!strcmp(value, ov));
          // If you want LSO's behaviour, remove the `!= 0`.
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(strcmp(value, ov) != 0);
        default:
          return nullptr;
      }
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Key Constants

LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator operation, LSLKeyConstant *cv, LSLConstant *other_const) {
  const char *value = cv->getValue();
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_STRING_CONSTANT:
    case NODE_KEY_CONSTANT: {
      const char *ov = ((LSLStringConstant *) other_const)->getValue();
      switch (operation) {
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(!strcmp(value, ov));
          // If you want LSO's behaviour, remove the `!= 0`.
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(strcmp(value, ov) != 0);
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
    LSLOperator operation, LSLListConstant *cv, LSLConstant *other_const) {
  // unary op
  if (other_const == nullptr) {
    return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_LIST_CONSTANT: {
      LSLListConstant *other = ((LSLListConstant *) other_const);
      switch (operation) {
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(
              cv->getLength() == other->getLength()
          );
        case OP_NEQ:
          // Yes, really.
          return _mAllocator->newTracked<LSLIntegerConstant>(
              cv->getLength() - other->getLength()
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
    LSLOperator operation, LSLVectorConstant *cv, LSLConstant *other_const) {
  const Vector3 *value = cv->getValue();
  // Make sure we have a value
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return _mAllocator->newTracked<LSLVectorConstant>(-value->x, -value->y, -value->z);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_INTEGER_CONSTANT: {
      int ov = ((LSLIntegerConstant *) other_const)->getValue();
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
      return _mAllocator->newTracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_FLOAT_CONSTANT: {
      float ov = ((LSLFloatConstant *) other_const)->getValue();
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
      return _mAllocator->newTracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    case NODE_VECTOR_CONSTANT: {
      const Vector3 *ov = ((LSLVectorConstant *) other_const)->getValue();
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
          return _mAllocator->newTracked<LSLFloatConstant>((value->x * ov->z) + (value->y * ov->y) + (value->z * ov->x));
        case '%':           // cross product
          nv[0] = (value->y * ov->z) - (value->z * ov->y);
          nv[1] = (value->z * ov->x) - (value->x * ov->z);
          nv[2] = (value->x * ov->y) - (value->y * ov->x);
          break;
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(*value == *ov);
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(*value != *ov);
        default:
          return nullptr;
      }
      return _mAllocator->newTracked<LSLVectorConstant>(nv[0], nv[1], nv[2]);
    }
    default:
      return nullptr;
  }
}

//////////////////////////////////////////////
// Quaternion Constants
LSLConstant *TailslideOperationBehavior::operation(
    LSLOperator operation, LSLQuaternionConstant *cv, LSLConstant *other_const) {
  const Quaternion *value = cv->getValue();
  if (value == nullptr)
    return nullptr;

  // unary op
  if (other_const == nullptr) {
    if (operation == '-')
      return _mAllocator->newTracked<LSLQuaternionConstant>(-value->x, -value->y, -value->z, -value->s);
    else
      return nullptr;
  }

  // binary op
  switch (other_const->getNodeSubType()) {
    case NODE_QUATERNION_CONSTANT: {
      const Quaternion *ov = ((LSLQuaternionConstant *) other_const)->getValue();
      if (ov == nullptr)
        return nullptr;
      switch (operation) {
        case OP_EQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(*value == *ov);
        case OP_NEQ:
          return _mAllocator->newTracked<LSLIntegerConstant>(*value != *ov);
        case '-':
          return _mAllocator->newTracked<LSLQuaternionConstant>(value->x - ov->x, value->y - ov->y, value->z - ov->z,
                                                               value->s - ov->s);
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

  auto orig_itype = cv->getIType();
  auto to_itype = to_type->getIType();

  if (orig_itype == to_itype) {
    // no-op case
    return cv;
  }

  // These may force us to do an expensive alloc (key <-> str is free)
  if ((to_itype == LST_STRING && orig_itype != LST_KEY) || to_itype == LST_LIST) {
    if (!_mMayCreateHeapValues)
      return nullptr;
  }

  LSLConstant *new_cv = nullptr;
  switch (orig_itype) {
    case LST_KEY:
      new_cv = cast(to_type, (LSLKeyConstant *)cv);
      break;
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
      new_cv = cast(to_type, (LSLListConstant *) cv);
      break;
    case LST_VECTOR:
      new_cv = cast(to_type, (LSLVectorConstant *)cv);
      break;
    case LST_QUATERNION:
      new_cv = cast(to_type, (LSLQuaternionConstant *)cv);
      break;
    case LST_MAX:
    case LST_NULL:
    case LST_ERROR:
      return nullptr;
  }
  if (new_cv)
    new_cv->setLoc(lloc);
  return new_cv;
}



LSLConstant *TailslideOperationBehavior::cast(LSLType *to_type, LSLStringConstant *cv) {
  auto *v = cv->getValue();
  switch(to_type->getIType()) {
    case LST_INTEGER: {
      int base = 10;
      // Need to explicitly determine what the base should be, we only support
      // base 10 and base 16 and we don't want `011` to be treated as octal!
      // This check is safe because `cv` must be a null terminated string.
      if (v[0] == '0' && (v[1] == 'x' || v[2] == 'X'))
        base = 16;
      return _mAllocator->newTracked<LSLIntegerConstant>((S32) strtoul(v, nullptr, base));
    }
    case LST_FLOATINGPOINT: {
      return _mAllocator->newTracked<LSLFloatConstant>((F32) strtod(v, nullptr));
    }
    case LST_KEY:
      return _mAllocator->newTracked<LSLKeyConstant>(v);
    default:
      return nullptr;
  }
}

LSLConstant *TailslideOperationBehavior::cast(LSLType *to_type, LSLKeyConstant *cv) {
  auto *v = cv->getValue();
  switch(to_type->getIType()) {
    case LST_STRING:
      return _mAllocator->newTracked<LSLStringConstant>(v);
    default:
      return nullptr;
  }
}

LSLConstant *TailslideOperationBehavior::cast(LSLType *to_type, LSLIntegerConstant *cv) {
  auto v = cv->getValue();
  switch(to_type->getIType()) {
    case LST_STRING: {
      return _mAllocator->newTracked<LSLStringConstant>(
          _mAllocator->copyStr(std::to_string(v).c_str())
      );
    }
    case LST_FLOATINGPOINT:
      return _mAllocator->newTracked<LSLFloatConstant>((float)v);
    default:
      return nullptr;
  }
}

const std::string INF_STR = "-inf";
const std::string NEG_INF_STR = "-inf";
const std::vector<std::string> NAN_STRS {
    "nan",
    "-nan",
    "-nan(ind)",
    "nan(ind)"
};

LSLConstant *TailslideOperationBehavior::cast(LSLType *to_type, LSLFloatConstant *cv) {
  auto v = cv->getValue();
  switch(to_type->getIType()) {
    case LST_STRING: {
      std::string f_as_str {std::to_string(v)};
      if (f_as_str == INF_STR)
        f_as_str = "Infinity";
      else if (f_as_str == NEG_INF_STR)
        f_as_str = "-Infinity";
        // Only one kind of NaN in LSL!
      else if (std::find(NAN_STRS.begin(), NAN_STRS.end(), f_as_str) != NAN_STRS.end())
        f_as_str = "NaN";

      return _mAllocator->newTracked<LSLStringConstant>(
          _mAllocator->copyStr(f_as_str.c_str())
      );
    }
    default:
      return nullptr;
  }
}

}
