#pragma once

struct YYLTYPE;

namespace Tailslide {
class LSLType;
class LSLConstant;
class LSLIntegerConstant;
class LSLStringConstant;
class LSLFloatConstant;
class LSLListConstant;
class LSLQuaternionConstant;
class LSLVectorConstant;

class AOperationBehavior {
  public:
    virtual LSLConstant *operation(
        int oper, LSLConstant *cv, LSLConstant *other_cv, YYLTYPE *lloc) = 0;
    virtual LSLConstant *cast(
        LSLType *to_type, LSLConstant *cv, YYLTYPE *lloc) = 0;
};

// Arbitrary operation behavior implemented by Tailslide itself. May not match the target platform's
// runtime operation behavior!
class TailslideOperationBehavior : public AOperationBehavior {
  public:
    // dispatch method
    LSLConstant *operation(
        int operation, LSLConstant *cv, LSLConstant *other_cv, YYLTYPE *lloc) override;

    // type-specific operation methods
    LSLConstant *operation(int operation, LSLIntegerConstant *cv, LSLConstant *other_cv);
    LSLConstant *operation(int operation, LSLFloatConstant *cv, LSLConstant *other_cv);
    LSLConstant *operation(int operation, LSLStringConstant *cv, LSLConstant *other_cv);
    LSLConstant *operation(int operation, LSLListConstant *cv, LSLConstant *other_cv);
    LSLConstant *operation(int operation, LSLVectorConstant *cv, LSLConstant *other_cv);
    LSLConstant *operation(int operation, LSLQuaternionConstant *cv, LSLConstant *other_cv);

    // dispatch method
    LSLConstant *cast(LSLType *to_type, LSLConstant *cv, YYLTYPE *lloc) override;

    // type-specific cast methods
    LSLConstant *cast(LSLType *to_type, LSLIntegerConstant *cv);
    LSLConstant *cast(LSLType *to_type, LSLFloatConstant *cv);
    LSLConstant *cast(LSLType *to_type, LSLStringConstant *cv);
    LSLConstant *cast(LSLType *to_type, LSLListConstant *cv) { return nullptr; };
    LSLConstant *cast(LSLType *to_type, LSLVectorConstant *cv) { return nullptr; };
    LSLConstant *cast(LSLType *to_type, LSLQuaternionConstant *cv) { return nullptr; };
};

}
