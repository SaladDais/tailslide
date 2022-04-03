#pragma once

struct YYLTYPE;

namespace Tailslide {
class LLScriptType;
class LLScriptConstant;
class LLScriptIntegerConstant;
class LLScriptStringConstant;
class LLScriptFloatConstant;
class LLScriptListConstant;
class LLScriptQuaternionConstant;
class LLScriptVectorConstant;

class AOperationBehavior {
  public:
    virtual LLScriptConstant *operation(
        int oper, LLScriptConstant *cv, LLScriptConstant *other_cv, YYLTYPE *lloc) = 0;
    virtual LLScriptConstant *cast(
        LLScriptType *to_type, LLScriptConstant *cv, YYLTYPE *lloc) = 0;
};

// Arbitrary operation behavior implemented by Tailslide itself. May not match the target platform's
// runtime operation behavior!
class TailslideOperationBehavior : public AOperationBehavior {
  public:
    // dispatch method
    LLScriptConstant *operation(
        int operation, LLScriptConstant *cv, LLScriptConstant *other_cv, YYLTYPE *lloc) override;

    // type-specific operation methods
    LLScriptConstant *operation(int operation, LLScriptIntegerConstant *cv, LLScriptConstant *other_cv);
    LLScriptConstant *operation(int operation, LLScriptFloatConstant *cv, LLScriptConstant *other_cv);
    LLScriptConstant *operation(int operation, LLScriptStringConstant *cv, LLScriptConstant *other_cv);
    LLScriptConstant *operation(
        int operation, LLScriptListConstant *cv, LLScriptConstant *other_cv, YYLTYPE *lloc);
    LLScriptConstant *operation(int operation, LLScriptVectorConstant *cv, LLScriptConstant *other_cv);
    LLScriptConstant *operation(int operation, LLScriptQuaternionConstant *cv, LLScriptConstant *other_cv);

    // dispatch method
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptConstant *cv, YYLTYPE *lloc) override;

    // type-specific cast methods
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptIntegerConstant *cv);
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptFloatConstant *cv);
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptStringConstant *cv);
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptListConstant *cv) { return nullptr; };
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptVectorConstant *cv) { return nullptr; };
    LLScriptConstant *cast(LLScriptType *to_type, LLScriptQuaternionConstant *cv) { return nullptr; };
};

}
