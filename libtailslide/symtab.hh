#ifndef _SYMTAB_HH
#define _SYMTAB_HH 1

#include <clocale>
#include <cstddef>
#include <vector>
#include <unordered_map>

#include "lslmini.tab.hh"
#include "allocator.hh"

namespace Tailslide {
enum LLSymbolType       { SYM_ANY = -1, SYM_VARIABLE, SYM_FUNCTION, SYM_STATE, SYM_LABEL, SYM_EVENT };
enum LLSymbolSubType    { SYM_LOCAL, SYM_GLOBAL, SYM_BUILTIN, SYM_FUNCTION_PARAMETER, SYM_EVENT_PARAMETER };

class LLScriptSymbol: public LLTrackableObject {
  public:
    LLScriptSymbol( const char *name, class LLScriptType *type, LLSymbolType symbol_type, LLSymbolSubType sub_type, YYLTYPE *lloc, class LLScriptFunctionDec *function_decl = NULL, class LLASTNode *var_decl = NULL )
      : name(name), type(type), symbol_type(symbol_type), sub_type(sub_type), lloc(*lloc), function_decl(function_decl), var_decl(var_decl),
      constant_value(NULL), references(0), assignments(0), mangled_name(NULL) {};

    LLScriptSymbol( const char *name, class LLScriptType *type, LLSymbolType symbol_type, LLSymbolSubType sub_type, class LLScriptFunctionDec *function_decl = NULL, class LLASTNode *var_decl = NULL )
      : name(name), type(type), symbol_type(symbol_type), sub_type(sub_type), function_decl(function_decl), var_decl(var_decl),
      constant_value(NULL), references(0), assignments(0), mangled_name(NULL) {};

    const char          *get_name()         { return name; }
    class LLScriptType  *get_type()         { return type; }

    int                  get_references()   { return references; }
    int                  add_reference()    { return ++references; }
    int                  get_assignments()  { return assignments; }
    int                  add_assignment()   { return ++assignments; }
    void                 reset_tracking()   { assignments = 0; references = 0; }

    LLSymbolType         get_symbol_type()  { return symbol_type; }
    LLSymbolSubType      get_sub_type()     { return sub_type;    }
    static const char         *get_type_name(LLSymbolType t)    {
      switch (t) {
        case SYM_VARIABLE:  return "variable";
        case SYM_FUNCTION:  return "function";
        case SYM_STATE:     return "state";
        case SYM_LABEL:     return "label";
        case SYM_ANY:       return "any";
        default:            return "invalid";
      }
    }

    YYLTYPE             *get_lloc()         { return &lloc; }
    class LLScriptFunctionDec *get_function_decl() { return function_decl; }
    class LLASTNode        *get_var_decl() { return var_decl; }

    class LLScriptConstant *get_constant_value()                            { return constant_value;    };
    void                    set_constant_value(class LLScriptConstant *v)   { constant_value = v;       };
    bool get_constant_precluded() { return constant_precluded; };
    void set_constant_precluded(bool precluded) { constant_precluded = precluded; };

    char                   *get_mangled_name()             { return mangled_name; };
    void                    set_mangled_name(char* m_name) {
      mangled_name = m_name;
    };

  private:
    const char          *name;
    class LLScriptType  *type;
    LLSymbolType         symbol_type;
    LLSymbolSubType      sub_type;
    YYLTYPE              lloc;
    class LLScriptFunctionDec *function_decl;
    class LLASTNode     *var_decl;
    class LLScriptConstant *constant_value;
    bool constant_precluded = false;
    int                  references;            // how many times this symbol is referred to
    int                  assignments;           // how many times it is assigned to
    char                *mangled_name;
};


// based on Java string hashing algo, assumes null-terminated
struct chash
  : std::unary_function<const char *, std::size_t>
{
  std::size_t operator()(const char *x) const
  {
    size_t result = 0;
    const size_t prime = 31;
    for(size_t i=0; x[i] != '\0'; ++i)
      result = x[i] + (result * prime);
    return result;
  }
};

struct cstr_equal_to
    : std::binary_function<const char *, const char *, bool>
{
  bool operator()(const char *x, const char *y) const
  {
    return strcmp(x, y) == 0;
  }
};

class LLScriptSymbolTable: public LLTrackableObject {
  public:
    LLScriptSymbol *lookup( const char *name, LLSymbolType type = SYM_ANY );
    void            define( LLScriptSymbol *symbol );
    bool            remove( LLScriptSymbol *symbol );
    void            check_symbols();
    void            register_subtable(LLScriptSymbolTable *table);
    void            unregister_subtable(LLScriptSymbolTable *table);
    void            reset_reference_data();
    void            set_mangled_names();

  private:
    typedef std::unordered_multimap<const char *, LLScriptSymbol *, chash, cstr_equal_to> SensitiveSymbolMap;

    // Vector to hold our symbolsa
    SensitiveSymbolMap symbols;

    // The root tables contains pointers to all of the tables
    // below it. This should be empty for anything else.
    std::vector<LLScriptSymbolTable *>  desc_tables;
};

}

#endif /* not _SYMTAB_HH */
