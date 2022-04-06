#ifndef _LOGGER_HH
#define _LOGGER_HH 1

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <utility>  // pair

#include "allocator.hh"
#include "lslmini.tab.hh"

namespace Tailslide {

enum LogLevel {
  LOG_ERROR,            // errors
  LOG_WARN,             // warnings
  LOG_INTERNAL_ERROR,   // internal errors
  LOG_INFO,             // what we're up to
  LOG_DEBUG,            // base debug messages
  LOG_DEBUG_MINOR,      // minor debug messages
  LOG_DEBUG_SPAM,       // spammy debug messages
  LOG_CONTINUE,         // continuation of last message
};

enum ErrorCode {
    // errors
    E_ERROR                 = 10000,
    E_DUPLICATE_DECLARATION = 10001,
    E_INVALID_OPERATOR = 10002,
    E_DEPRECATED = 10003,
    E_DEPRECATED_WITH_REPLACEMENT = 10004,
    E_WRONG_TYPE = 10005,
    E_UNDECLARED = 10006,
    E_UNDECLARED_WITH_SUGGESTION = 10007,
    E_INVALID_MEMBER = 10008,
    E_MEMBER_NOT_VARIABLE = 10009,
    E_MEMBER_WRONG_TYPE = 10010,
    E_ARGUMENT_WRONG_TYPE = 10011,
    E_TOO_MANY_ARGUMENTS = 10012,
    E_TOO_FEW_ARGUMENTS = 10013,
    E_CHANGE_STATE_IN_FUNCTION = 10014,
    E_WRONG_TYPE_IN_ASSIGNMENT = 10015,
    E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT = 10016,
    E_RETURN_VALUE_IN_EVENT_HANDLER = 10017,
    E_BAD_RETURN_TYPE = 10018,
    E_NOT_ALL_PATHS_RETURN = 10019,
    E_SYNTAX_ERROR = 10020,
    E_GLOBAL_INITIALIZER_NOT_CONSTANT = 10021,
    E_NO_OPERATOR = 10022,
    E_NO_EVENT_HANDLERS = 10023,
    E_PARSER_STACK_DEPTH = 10024,
    E_BUILTIN_LVALUE = 10025,
    E_SHADOW_CONSTANT = 10026,
    E_ARGUMENT_WRONG_TYPE_EVENT = 10027,
    E_TOO_MANY_ARGUMENTS_EVENT = 10028,
    E_TOO_FEW_ARGUMENTS_EVENT = 10029,
    E_INVALID_EVENT = 10030,
    E_EVENT_AS_IDENTIFIER = 10031,
    E_DECLARATION_INVALID_HERE = 10032,
    E_MULTIPLE_EVENT_HANDLERS = 10033,
    E_LIST_IN_LIST = 10034,
    E_ILLEGAL_CAST = 10035,
    E_LAST,




    // warnings
    W_WARNING               = 20000,
    W_SHADOW_DECLARATION = 20001,
    W_ASSIGNMENT_IN_COMPARISON = 20002,
    W_CHANGE_TO_CURRENT_STATE = 20003,
    W_CHANGE_STATE_HACK_CORRUPT = 20004,
    W_CHANGE_STATE_HACK = 20005,
    W_DEPRECATED_1 = 20006,
    W_EMPTY_IF = 20007,
    W_BAD_DECIMAL_LEX = 20008,
    W_DECLARED_BUT_NOT_USED = 20009,
    W_UNUSED_EVENT_PARAMETER = 20010,
    W_LIST_COMPARE = 20011,
    W_CONDITION_ALWAYS_TRUE = 20012,
    W_CONDITION_ALWAYS_FALSE = 20013,
    W_EMPTY_LOOP = 20014,
    W_INT_FLOAT_MUL_ASSIGN = 20015,
    W_JUMP_TO_WRONG_LABEL = 20016,
    W_DUPLICATE_LABEL_NAME = 20017,
    W_LAST,
};

#ifndef HIDE_TAILSLIDE_INTERNALS
#define LINECOL(l)  (l)->first_line, (l)->first_column
#define NODE_ERROR(node, ...) do {(node)->context->logger->error((node)->get_lloc(), __VA_ARGS__);} while(0)
#endif

class Logger {
  public:
    explicit Logger(ScriptAllocator *allocator) :
               errors(0), warnings(0), show_end(false), show_info(false), sort(true),
               show_error_codes(true), check_assertions(false), last_message(NULL),
               file(stderr), finalized(false), _allocator(allocator) {};
    void log(LogLevel type, YYLTYPE *loc, const char *fmt, ...);
    void logv(LogLevel type, YYLTYPE *loc, const char *fmt, va_list args, int error=0);
    void error( YYLTYPE *loc, int error, ... );
    void report();
    void reset();
    void finalize();

    int     get_errors()    { return errors;    }
    int     get_warnings()  { finalize(); return warnings;  }
    void    set_show_end(bool v) { show_end = v; }
    void    set_show_info(bool v){ show_info = v;}
    void    set_sort(bool v)     { sort = v;     }
    void    set_show_error_codes(bool v) { show_error_codes = v; }
    void    set_check_assertions(bool v) { check_assertions = v; }
    void    filter_assert_errors();

    void    add_assertion( int line, ErrorCode error ) {
      assertions.emplace_back( std::pair<int, ErrorCode>( line, error ) );
    }

  protected:
    int     errors;
    int     warnings;
    bool    show_end;
    bool    show_info;
    bool    sort;
    bool    show_error_codes;
    bool    check_assertions;
    bool    finalized;
    class LogMessage *last_message;
    ScriptAllocator *_allocator;

  private:
    static thread_local Logger instance;
    FILE    *file;
    std::vector<class LogMessage*>    messages;
    std::vector<ErrorCode>            errors_seen;
    std::vector< std::pair<int, ErrorCode>>    assertions;
    static const char* error_messages[];
    static const char* warning_messages[];
};

inline void DEBUG(LogLevel level, YYLTYPE *yylloc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  //Logger::get()->logv(level, yylloc, fmt, args);
  va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// Log message entry, for sorting
class LogMessage: public TrackableObject {
  public:
    LogMessage( ScriptContext *ctx, LogLevel type, YYLTYPE *loc, char *message, ErrorCode error );

    LogLevel    get_type() { return type; }
    YYLTYPE    *get_loc()  { return &loc;  }
    ErrorCode   get_error() { return error; }
    void        cont(char *message);
    void        print(FILE *fp);

  private:
    LogLevel            type;

    // we need our own copy of loc, because messages logged in the parser will be
    // handing us a copy of a loc structure that is constantly changing, and will
    // be invalid when we go to sort.
    YYLTYPE             loc;

    std::vector<char*>  messages;
    ErrorCode           error;
};

}

#endif
