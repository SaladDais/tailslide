#pragma once

#include <vector>

#include "lslmini.hh"
#include "visitor.hh"
#include "bitstream.hh"
#include "bytecode_format.hh"

namespace Tailslide {

/// LSO-specific bitstream with LSO-specific serialization helpers
class LSOBitStream : public BitStream {
  public:
    explicit LSOBitStream(Endianness endian=ENDIAN_BIG) : BitStream(endian) {}
    LSOBitStream(const uint8_t *data, const uint32_t length, Endianness endian=ENDIAN_BIG): BitStream(data, length, endian) {}
    LSOBitStream(LSOBitStream &&other) noexcept: BitStream(std::move(other)) {}
    LSOBitStream(const LSOBitStream &other) = delete;
};


inline LSOBitStream& operator<<(LSOBitStream &bs, const Vector3 &v) {
  bs << v.z;
  bs << v.y;
  bs << v.x;
  return bs;
}

inline LSOBitStream& operator>>(LSOBitStream &bs, Vector3 &v) {
  bs >> v.z;
  bs >> v.y;
  bs >> v.x;
  return bs;
}

inline LSOBitStream& operator<<(LSOBitStream &bs, const Quaternion &v) {
  bs << v.s;
  bs << v.z;
  bs << v.y;
  bs << v.x;
  return bs;
}

inline LSOBitStream& operator>>(LSOBitStream &bs, Quaternion &v) {
  bs >> v.s;
  bs >> v.z;
  bs >> v.y;
  bs >> v.x;
  return bs;
}


class LSOHeapManager {
  public:
    uint32_t writeConstant(LSLConstant *constant);
    uint32_t writeTerminalBlock();
    LSOBitStream mHeapBS {ENDIAN_BIG};
  protected:
    void writeHeader(uint32_t size, LSLIType type, uint16_t ref_count=1);
};

class LSOGlobalVarManager {
  public:
    explicit LSOGlobalVarManager(LSOHeapManager *heap_manager): _mHeapManager(heap_manager) {}
    void writeVar(LSLSymbol *symbol);
    void writePlaceholder(LSLIType type);
    LSOBitStream mGlobalsBS {ENDIAN_BIG};
  protected:
    LSOHeapManager *_mHeapManager;
};

class LSOCompilerVisitor : public ASTVisitor {
  public:
    LSOBitStream mScriptBS {ENDIAN_BIG};
  protected:
    virtual bool visit(LSLScript *node);
    virtual bool visit(LSLGlobalVariable *node);

    void writeRegister(LSORegisters reg, uint32_t val);
    void writeEventRegister(LSORegisters reg, uint64_t val);

    LSOBitStream _mFunctionsBS {ENDIAN_BIG};
    LSOBitStream _mStatesBS {ENDIAN_BIG};
    LSOHeapManager _mHeapManager;
    LSOGlobalVarManager _mGlobalVarManager {&_mHeapManager};
};

}
