#ifndef TAILSLIDE_SYMTAB_HH
#define TAILSLIDE_SYMTAB_HH

#include <clocale>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include "unordered_cstr_map.hh"

#include "allocator.hh"

namespace Tailslide {

enum LSLIType : uint8_t {
  LST_NULL          = 0,
  LST_INTEGER       = 1,
  LST_FLOATINGPOINT = 2,
  LST_STRING        = 3,
  LST_KEY           = 4,
  LST_VECTOR        = 5,
  LST_QUATERNION    = 6,
  LST_LIST          = 7,
  LST_ERROR         = 8,   // special value so processing can continue without throwing bogus errors
  LST_MAX           = 9,
};

enum LSLSymbolType       { SYM_ANY = -1, SYM_VARIABLE, SYM_FUNCTION, SYM_STATE, SYM_LABEL, SYM_EVENT };
enum LSLSymbolSubType    { SYM_LOCAL, SYM_GLOBAL, SYM_BUILTIN, SYM_FUNCTION_PARAMETER, SYM_EVENT_PARAMETER };

class LSLSymbol: public TrackableObject {
  public:
    LSLSymbol( ScriptContext *ctx, const char *name, class LSLType *type, LSLSymbolType symbol_type, LSLSymbolSubType sub_type, YYLTYPE *lloc, class LSLParamList *function_decl = NULL, class LSLASTNode *var_decl = NULL )
      : _mName(name), _mType(type), _mSymbolType(symbol_type), _mSubType(sub_type), _mLoc(*lloc), _mFunctionDecl(function_decl), _mVarDecl(var_decl),
        _mConstantValue(NULL), _mReferences(0), _mAssignments(0), _mMangledName(NULL), TrackableObject(ctx) {};

    LSLSymbol( ScriptContext *ctx, const char *name, class LSLType *type, LSLSymbolType symbol_type, LSLSymbolSubType sub_type, class LSLParamList *function_decl = NULL, class LSLASTNode *var_decl = NULL )
      : _mName(name), _mType(type), _mSymbolType(symbol_type), _mSubType(sub_type), _mFunctionDecl(function_decl), _mVarDecl(var_decl),
        _mConstantValue(NULL), _mReferences(0), _mAssignments(0), _mMangledName(NULL), _mLoc({}), TrackableObject(ctx) {};

    const char          *getName()         { return _mName; }
    class LSLType  *getType()         { return _mType; }
    LSLIType getIType();

    int                  getReferences() const   { return _mReferences; }
    int                  addReference()    { return ++_mReferences; }
    int                  getAssignments() const  { return _mAssignments; }
    int                  addAssignment()   { return ++_mAssignments; }
    void                 resetTracking()   { _mAssignments = 0; _mReferences = 0; }

    LSLSymbolType         getSymbolType()  { return _mSymbolType; }
    LSLSymbolSubType      getSubType()     { return _mSubType;    }
    static const char         *getTypeName(LSLSymbolType t)    {
      switch (t) {
        case SYM_VARIABLE:  return "variable";
        case SYM_FUNCTION:  return "function";
        case SYM_STATE:     return "state";
        case SYM_LABEL:     return "label";
        case SYM_ANY:       return "any";
        default:            return "invalid";
      }
    }

    YYLTYPE             *getLoc()         { return &_mLoc; }
    class LSLParamList *getFunctionDecl() { return _mFunctionDecl; }
    class LSLASTNode        *getVarDecl() { return _mVarDecl; }

    class LSLConstant *getConstantValue()                       { return _mConstantValue;    };
    void               setConstantValue(class LSLConstant *v)   { _mConstantValue = v;       };
    bool getConstantPrecluded() const { return _mConstantPrecluded; };
    void setConstantPrecluded(bool precluded) { _mConstantPrecluded = precluded; };

    char                   *getMangledName()             { return _mMangledName; };
    void                    setMangledName(char *m_name) { _mMangledName = m_name; };

    bool getAllPathsReturn() const { return _mAllPathsReturn; }
    void setAllPathsReturn(bool all_ret) { _mAllPathsReturn = all_ret; }

  private:
    const char          *_mName;
    class LSLType  *_mType;
    LSLSymbolType         _mSymbolType;
    LSLSymbolSubType      _mSubType;
    YYLTYPE              _mLoc;
    class LSLParamList *_mFunctionDecl;
    class LSLASTNode     *_mVarDecl;
    class LSLConstant *_mConstantValue;
    bool _mConstantPrecluded = false;
    int                  _mReferences;            // how many times this symbol is referred to
    int                  _mAssignments;           // how many times it is assigned to
    char                *_mMangledName;
    bool _mAllPathsReturn = false;
};

class LSLSymbolTable: public TrackableObject {
  public:
    explicit LSLSymbolTable(ScriptContext *ctx): TrackableObject(ctx) {};
    LSLSymbol *lookup( const char *name, LSLSymbolType type = SYM_ANY );
    void            define( LSLSymbol *symbol );
    bool            remove( LSLSymbol *symbol );
    void            checkSymbols();
    void resetTracking();

  private:
    UnorderedCStrMap<LSLSymbol*> _mSymbols;
    LSLSymbolTable *_mParent = nullptr;

  public:
    UnorderedCStrMap<LSLSymbol*> &getMap() {return _mSymbols;}
};

class LSLSymbolTableManager {
  public:
    explicit LSLSymbolTableManager(ScriptAllocator *allocator) {_mAllocator = allocator;};
    void registerTable(LSLSymbolTable *table) {_mTables.push_back(table);};
    void setMangledNames();
    void resetTracking();
  protected:
    std::vector<LSLSymbolTable *> _mTables {};
    ScriptAllocator *_mAllocator;
};

}

#endif
