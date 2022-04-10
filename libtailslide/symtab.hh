#ifndef _SYMTAB_HH
#define _SYMTAB_HH 1

#include <clocale>
#include <cstddef>
#include <vector>
#include <unordered_map>

#include "lslmini.tab.hh"
#include "allocator.hh"

namespace Tailslide {
enum LSLSymbolType       { SYM_ANY = -1, SYM_VARIABLE, SYM_FUNCTION, SYM_STATE, SYM_LABEL, SYM_EVENT };
enum LSLSymbolSubType    { SYM_LOCAL, SYM_GLOBAL, SYM_BUILTIN, SYM_FUNCTION_PARAMETER, SYM_EVENT_PARAMETER };

class LSLSymbol: public TrackableObject {
  public:
    LSLSymbol( ScriptContext *ctx, const char *name, class LSLType *type, LSLSymbolType symbol_type, LSLSymbolSubType sub_type, YYLTYPE *lloc, class LSLFunctionDec *function_decl = NULL, class LSLASTNode *var_decl = NULL )
      : name(name), type(type), symbol_type(symbol_type), sub_type(sub_type), lloc(*lloc), function_decl(function_decl), var_decl(var_decl),
      constant_value(NULL), references(0), assignments(0), mangled_name(NULL), TrackableObject(ctx) {};

    LSLSymbol( ScriptContext *ctx, const char *name, class LSLType *type, LSLSymbolType symbol_type, LSLSymbolSubType sub_type, class LSLFunctionDec *function_decl = NULL, class LSLASTNode *var_decl = NULL )
      : name(name), type(type), symbol_type(symbol_type), sub_type(sub_type), function_decl(function_decl), var_decl(var_decl),
      constant_value(NULL), references(0), assignments(0), mangled_name(NULL), TrackableObject(ctx) {};

    const char          *get_name()         { return name; }
    class LSLType  *get_type()         { return type; }

    int                  get_references()   { return references; }
    int                  add_reference()    { return ++references; }
    int                  get_assignments()  { return assignments; }
    int                  add_assignment()   { return ++assignments; }
    void                 reset_tracking()   { assignments = 0; references = 0; }

    LSLSymbolType         get_symbol_type()  { return symbol_type; }
    LSLSymbolSubType      get_sub_type()     { return sub_type;    }
    static const char         *get_type_name(LSLSymbolType t)    {
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
    class LSLFunctionDec *get_function_decl() { return function_decl; }
    class LSLASTNode        *get_var_decl() { return var_decl; }

    class LSLConstant *get_constant_value()                            { return constant_value;    };
    void                    set_constant_value(class LSLConstant *v)   { constant_value = v;       };
    bool get_constant_precluded() { return constant_precluded; };
    void set_constant_precluded(bool precluded) { constant_precluded = precluded; };

    char                   *get_mangled_name()             { return mangled_name; };
    void                    set_mangled_name(char* m_name) {
      mangled_name = m_name;
    };

  private:
    const char          *name;
    class LSLType  *type;
    LSLSymbolType         symbol_type;
    LSLSymbolSubType      sub_type;
    YYLTYPE              lloc;
    class LSLFunctionDec *function_decl;
    class LSLASTNode     *var_decl;
    class LSLConstant *constant_value;
    bool constant_precluded = false;
    int                  references;            // how many times this symbol is referred to
    int                  assignments;           // how many times it is assigned to
    char                *mangled_name;
};


// based on Java string hashing algo, assumes null-terminated
template <class T = const char *>
struct chash {
  constexpr std::size_t operator()(T x) const {
    size_t result = 0;
    const size_t prime = 31;
    for(size_t i=0; x[i] != '\0'; ++i)
      result = x[i] + (result * prime);
    return result;
  }
};

template <class T = const char *>
struct cstr_equal_to {
  constexpr bool operator()(T x, T y) const {
    return strcmp(x, y) == 0;
  }
};

class LSLSymbolTable: public TrackableObject {
  public:
    LSLSymbolTable(ScriptContext *ctx): TrackableObject(ctx) {};
    LSLSymbol *lookup( const char *name, LSLSymbolType type = SYM_ANY );
    void            define( LSLSymbol *symbol );
    bool            remove( LSLSymbol *symbol );
    void            check_symbols();
    void            register_subtable(LSLSymbolTable *table);
    void            unregister_subtable(LSLSymbolTable *table);
    void            reset_reference_data();
    void            set_mangled_names();

  private:
    typedef std::unordered_multimap<
        const char *,
        LSLSymbol *,
        chash<const char *>,
        cstr_equal_to<const char *>
      > SensitiveSymbolMap;

    SensitiveSymbolMap symbols;

    // The root table contains pointers to all of the tables
    // below it. This should be empty for anything else.
    // TODO: This symbol table parenting logic sucks. replace it.
    std::vector<LSLSymbolTable *>  desc_tables;

  public:
    SensitiveSymbolMap &get_map() {return symbols;}
};

}

#endif /* not _SYMTAB_HH */
