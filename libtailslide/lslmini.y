%{
    #include "lslmini.hh"
    #include "logger.hh"
    #include <stdio.h>
    #include <string.h>

    using namespace Tailslide;
    //int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param);
    extern int tailslide_lex (TAILSLIDE_STYPE * yylval_param, TAILSLIDE_LTYPE * yylloc_param , void *yyscanner);
    extern ScriptContext *tailslide_get_extra(void *scanner);
    #define ALLOCATOR tailslide_get_extra(scanner)->allocator
%}
%{
    int yyerror( YYLTYPE*, void *, const char * );
    #define MAKEID(type,id,pos) ALLOCATOR->new_tracked<LSLIdentifier>(TYPE(type), (id), &(pos))
    // Anything larger is liable to make MSVC explode with the default compile options
    #ifndef LSLINT_STACK_OVERFLOW_AT
    #  ifdef _WIN32
    #    define LSLINT_STACK_OVERFLOW_AT 1000
    #  else
    #    define LSLINT_STACK_OVERFLOW_AT 10000
    #  endif
    #endif
    // slightly higher so we can still have assert comments that check for stack depth
    #define YYMAXDEPTH LSLINT_STACK_OVERFLOW_AT + 20
    #define YYINITDEPTH YYMAXDEPTH
    inline int _yylex( TAILSLIDE_STYPE * yylval, YYLTYPE *yylloc, void *yyscanner, int stack ) {
        if ( stack == LSLINT_STACK_OVERFLOW_AT ) {
            tailslide_get_extra(yyscanner)->logger->error( yylloc, E_PARSER_STACK_DEPTH );
            return 0;
        }
        return tailslide_lex( yylval, yylloc, yyscanner );
    }
    #ifdef yylex
    #  undef yylex
    #endif
    #define yylex(a,b,c) _yylex(a, b, c,  (int)(yyssp - yyss))
        

    // Same as bison's default, but update global position so we don't have
    // to pass it in every time we make a branch
    # define YYLLOC_DEFAULT(Current, Rhs, N)                \
        ((Current).first_line   = (Rhs)[1].first_line,       \
         (Current).first_column = (Rhs)[1].first_column,     \
         (Current).last_line    = (Rhs)[N].last_line,        \
         (Current).last_column  = (Rhs)[N].last_column,      \
         tailslide_get_extra(scanner)->glloc = (Current))

%}

%define api.prefix {tailslide_}
%define parse.error verbose
%locations
%define api.pure
%parse-param { void *scanner }
%lex-param { void *scanner }

%code requires {
#include "loctype.hh"

    namespace Tailslide {
        typedef int   S32;
        typedef float F32;
        class LSLType;
        class LSLConstant;
        class LSLIdentifier;
        class LSLGlobalVariable;
        class LSLEvent;
        class LSLEventHandler;
        class LSLExpression;
        class LSLStatement;
        class LSLGlobalFunction;
        class LSLFunctionDec;
        class LSLEventDec;
        class LSLASTNodeList;
        class LSLState;
        class LSLGlobalStorage;
        class LSLScript;
    }
}

%union
{
    Tailslide::S32                             ival;
    Tailslide::F32                             fval;
    char                                       *sval;
    class Tailslide::LSLType              *type;
    class Tailslide::LSLConstant          *constant;
    class Tailslide::LSLIdentifier        *identifier;
    class Tailslide::LSLGlobalVariable    *global;
    class Tailslide::LSLEvent             *event;
    class Tailslide::LSLEventHandler      *handler;
    class Tailslide::LSLExpression        *expression;
    class Tailslide::LSLStatement         *statement;
    class Tailslide::LSLGlobalFunction    *global_funcs;
    class Tailslide::LSLFunctionDec       *global_decl;
    class Tailslide::LSLEventDec          *global_event_decl;
    class Tailslide::LSLASTNodeList       *node_list;
    class Tailslide::LSLState             *state;
    class Tailslide::LSLGlobalStorage     *global_store;
    class Tailslide::LSLScript            *script;
};


%token                    INTEGER
%token                    FLOAT_TYPE
%token                    STRING
%token                    LLKEY
%token                    VECTOR
%token                    QUATERNION
%token                    LIST

%token                    STATE
%token                    EVENT
%token                    JUMP
%token                    RETURN

%token <sval>             IDENTIFIER
%token <sval>             STATE_DEFAULT

%token <ival>             INTEGER_CONSTANT

%token <fval>             FP_CONSTANT

%token <sval>             STRING_CONSTANT

%token                    INC_OP
%token                    DEC_OP
%token                    INC_PRE_OP
%token                    DEC_PRE_OP
%token                    INC_POST_OP
%token                    DEC_POST_OP
%token                    ADD_ASSIGN
%token                    SUB_ASSIGN
%token                    MUL_ASSIGN
%token                    DIV_ASSIGN
%token                    MOD_ASSIGN

%token                    EQ
%token                    NEQ
%token                    GEQ
%token                    LEQ

%token                    BOOLEAN_AND
%token                    BOOLEAN_OR

%token                    SHIFT_LEFT
%token                    SHIFT_RIGHT

%token                    IF
%token                    ELSE
%token                    FOR
%token                    DO
%token                    WHILE

%token                    PRINT

%token                    PERIOD

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%type <script>            lscript_program
%type <global_store>      globals
%type <global_store>      global
%type <global>            global_variable
%type <constant>          constant
%type <type>              typename
%type <global_funcs>      global_function
%type <global_decl>       function_parameters
%type <global_decl>       function_parameter
%type <global_event_decl> event_parameters
%type <global_event_decl> event_parameter
%type <state>             states
%type <state>             other_states
%type <state>             default
%type <state>             state
%type <handler>           state_body
%type <handler>           event
%type <statement>         compound_statement
%type <statement>         statement
%type <statement>         statements
%type <statement>         declaration
%type <statement>         ';'
%type <statement>         '@'
%type <expression>        nextforexpressionlist
%type <node_list>         forexpressionlist
%type <expression>        nextfuncexpressionlist
%type <expression>        funcexpressionlist
%type <expression>        nextlistexpressionlist
%type <expression>        listexpressionlist
%type <expression>        unarypostfixexpression
%type <expression>        vector_initializer
%type <expression>        quaternion_initializer
%type <expression>        list_initializer
%type <expression>        lvalue
%type <expression>        '-'
%type <expression>        '!'
%type <expression>        '~'
%type <expression>        '='
%type <expression>        '<'
%type <expression>        '>'
%type <expression>        '+'
%type <expression>        '*'
%type <expression>        '/'
%type <expression>        '%'
%type <expression>        '&'
%type <expression>        '|'
%type <expression>        '^'
%type <expression>        ADD_ASSIGN
%type <expression>        SUB_ASSIGN
%type <expression>        MUL_ASSIGN
%type <expression>        DIV_ASSIGN
%type <expression>        MOD_ASSIGN
%type <expression>        EQ
%type <expression>        NEQ
%type <expression>        LEQ
%type <expression>        GEQ
%type <expression>        BOOLEAN_AND
%type <expression>        BOOLEAN_OR
%type <expression>        SHIFT_LEFT
%type <expression>        SHIFT_RIGHT
%type <expression>        INC_OP
%type <expression>        DEC_OP
%type <expression>        '('
%type <expression>        ')'
%type <expression>        PRINT
%type <identifier>        name_type
%type <expression>        expression
%type <expression>        unaryexpression
%type <expression>        typecast

%right    '=' MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%left     BOOLEAN_AND BOOLEAN_OR
%left     '|'
%left     '^'
%left     '&'
%left     EQ NEQ
%left     '<' LEQ '>' GEQ
%left     SHIFT_LEFT SHIFT_RIGHT
%left     '+' '-'
%left     '*' '/' '%'
%right    '!' '~' INC_OP DEC_OP
%nonassoc INITIALIZER

%%

lscript_program
    : globals states
    {
        tailslide_get_extra(scanner)->script = ALLOCATOR->new_tracked<LSLScript>(
            ALLOCATOR->new_tracked<LSLASTNodeList>($1),
            ALLOCATOR->new_tracked<LSLASTNodeList>($2)
        );
    }
    | states
    {
        tailslide_get_extra(scanner)->script = ALLOCATOR->new_tracked<LSLScript>(
            ALLOCATOR->new_tracked<LSLASTNodeList>(),
            ALLOCATOR->new_tracked<LSLASTNodeList>($1)
        );
    }
    ;

globals
    : global
    {
        DEBUG( LOG_DEBUG_SPAM, nullptr, "** global\n");
        $$ = $1;
    }
    | global globals
    {
        if ( $1 ) {
            DEBUG( LOG_DEBUG_SPAM, nullptr, "** global [%p,%p] globals [%p,%p]\n", $1->get_prev(), $1->get_next(), $2->get_prev(), $2->get_next());
            $1->add_next_sibling($2);
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    ;

global
    : global_variable
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalStorage>($1, nullptr);
    }
    | global_function
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalStorage>(nullptr, $1);
    }
    ;

name_type
    : typename IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLIdentifier>($1, $2, &@2);
    }
    ;

global_variable
    : name_type ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalVariable>($1, nullptr);
    }
    | name_type '=' expression ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalVariable>($1, $3);
    }
    | name_type '=' error ';'
    {
        $$ = nullptr;
    }
    ;

constant
    : '-' INTEGER_CONSTANT
    {
        $$ = ALLOCATOR->new_tracked<LSLIntegerConstant>(-$2);
    }
    | INTEGER_CONSTANT
    {
        $$ = ALLOCATOR->new_tracked<LSLIntegerConstant>($1);
    }
    | '-' FP_CONSTANT
    {
        $$ = ALLOCATOR->new_tracked<LSLFloatConstant>(-$2);
    }
    | FP_CONSTANT
    {
        $$ = ALLOCATOR->new_tracked<LSLFloatConstant>($1);
    }
    | STRING_CONSTANT
    {
        $$ = ALLOCATOR->new_tracked<LSLStringConstant>($1);
    }
    ;

typename
    : INTEGER
    {
        $$ = TYPE(LST_INTEGER);
    }
    | FLOAT_TYPE
    {
        $$ = TYPE(LST_FLOATINGPOINT);
    }
    | STRING
    {
        $$ = TYPE(LST_STRING);
    }
    | LLKEY
    {
        $$ = TYPE(LST_KEY);
    }
    | VECTOR
    {
        $$ = TYPE(LST_VECTOR);
    }
    | QUATERNION
    {
        $$ = TYPE(LST_QUATERNION);
    }
    | LIST
    {
        $$ = TYPE(LST_LIST);
    }
    ;

global_function
    : IDENTIFIER '(' ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalFunction>( MAKEID(LST_NULL, $1, @1), nullptr, $4 );
    }
    | name_type '(' ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalFunction>( $1, nullptr, $4 );
    }
    | IDENTIFIER '(' function_parameters ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalFunction>( MAKEID(LST_NULL, $1, @1), $3, $5 );
    }
    | name_type '(' function_parameters ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLGlobalFunction>( $1, $3, $5 );
    }
    ;

function_parameters
    : function_parameter
    {
        $$ = $1;
    }
    | function_parameter ',' function_parameters
    {
        if ( $1 ) {
            $1->push_child($3->get_children());
            // delete $3;
            $$ = $1;
        } else {
            $$ = $3;
        }
    }
    ;

function_parameter
    : typename IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLFunctionDec>( ALLOCATOR->new_tracked<LSLIdentifier>($1, $2, &@2) );
    }
    ;

event_parameters
    : event_parameter
    {
        $$ = $1;
    }
    | event_parameter ',' event_parameters
    {
        if ( $1 ) {
            $1->push_child($3->get_children());
            // delete $3;
            $$ = $1;
        } else {
            $$ = $3;
        }
    }
    ;

event_parameter
    : typename IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLEventDec>( ALLOCATOR->new_tracked<LSLIdentifier>($1, $2, &@2) );
    }
    ;

states
    : default
    {
        $$ = $1;
    }
    | default other_states
    {
        if ( $1 ) {
            DEBUG( LOG_DEBUG_SPAM, nullptr, "---- default [%p,%p] other_states [%p,%p]\n", $1->get_prev(), $1->get_next(), $2->get_prev(), $2->get_next());
            $1->add_next_sibling($2);
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    ;

other_states
    : state
    {
        //DEBUG(200,"--(%d)-- state\n", yylloc.first_line);
        $$ = $1;
    }
    | state other_states
    {
        //DEBUG(200,"--(%d)-- state other_states\n", yylloc.first_line);
        if ( $1 ) {
            $1->add_next_sibling($2);
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    ;

default
    : STATE_DEFAULT '{' state_body '}'
    {
        $$ = ALLOCATOR->new_tracked<LSLState>( nullptr, $3 );
    }
    | STATE_DEFAULT '{' '}'
    {
        tailslide_get_extra(scanner)->logger->error( &@1, E_NO_EVENT_HANDLERS );
        $$ = ALLOCATOR->new_tracked<LSLState>( nullptr, nullptr );
    }
    ;

state
    : STATE IDENTIFIER '{' state_body '}'
    {
        $$ = ALLOCATOR->new_tracked<LSLState>( MAKEID(LST_NULL, $2, @2), $4 );
    }
    | STATE IDENTIFIER '{' '}'
    {
        tailslide_get_extra(scanner)->logger->error( &@1, E_NO_EVENT_HANDLERS );
        $$ = ALLOCATOR->new_tracked<LSLState>( nullptr, nullptr );
    }
    ;

state_body
    : event
    {
        $$ = $1;
    }
    | event state_body
    {
        if ( $1 ) {
            $1->add_next_sibling($2);
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    ;

event
    : IDENTIFIER '(' ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLEventHandler>(MAKEID(LST_NULL, $1, @1), nullptr, $4);
    }
    | IDENTIFIER '(' event_parameters ')' compound_statement
    {
        $$ = ALLOCATOR->new_tracked<LSLEventHandler>(MAKEID(LST_NULL, $1, @1), $3, $5);
    }
   ;

compound_statement
    : '{' '}'
    {
        $$ = ALLOCATOR->new_tracked<LSLCompoundStatement>(nullptr);
    }
    | '{' statements '}'
    {
        $$ = ALLOCATOR->new_tracked<LSLCompoundStatement>($2);
    }
    ;

statements
    : statement
    {
        //DEBUG( LOG_DEBUG_SPAM, nullptr, "statement %d\n", yylloc.first_line );
        $$ = $1;
    }
    | statements statement
    {
        if ( $1 ) {
            $1->add_next_sibling($2);
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    ;

statement
    : ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLStatement>(nullptr);
    }
    | STATE IDENTIFIER ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLStateStatement>(MAKEID(LST_NULL, $2, @2));
    }
    | STATE STATE_DEFAULT ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLStateStatement>();
    }
    | JUMP IDENTIFIER ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLJumpStatement>(MAKEID(LST_NULL, $2, @2));
    }
    | '@' IDENTIFIER ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLLabel>(MAKEID(LST_NULL, $2, @2));
    }
    | RETURN expression ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLReturnStatement>($2);
    }
    | RETURN ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLReturnStatement>(nullptr);
    }
    | expression ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLStatement>($1);
    }
    | declaration ';'
    {
        $$ = $1;
    }
    | compound_statement
    {
        $$ = $1;
    }
    | IF '(' expression ')' statement    %prec LOWER_THAN_ELSE
    {
        $$ = ALLOCATOR->new_tracked<LSLIfStatement>($3, $5, nullptr);
        $5->set_declaration_allowed(false);
    }
    | IF '(' expression ')' statement ELSE statement
    {
        $$ = ALLOCATOR->new_tracked<LSLIfStatement>($3, $5, $7);
        $5->set_declaration_allowed(false);
        $7->set_declaration_allowed(false);
    }
    | FOR '(' forexpressionlist ';' expression ';' forexpressionlist ')' statement
    {
        $$ = ALLOCATOR->new_tracked<LSLForStatement>($3, $5, $7, $9);
        $9->set_declaration_allowed(false);
    }
    | DO statement WHILE '(' expression ')' ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLDoStatement>($2, $5);
        $2->set_declaration_allowed(false);
    }
    | WHILE '(' expression ')' statement
    {
        $$ = ALLOCATOR->new_tracked<LSLWhileStatement>($3, $5);
        $5->set_declaration_allowed(false);
    }
    | error ';'
    {
        $$ = ALLOCATOR->new_tracked<LSLStatement>(nullptr);
    }
    ;

declaration
    : typename IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLDeclaration>(ALLOCATOR->new_tracked<LSLIdentifier>($1, $2, &@2), nullptr);
    }
    | typename IDENTIFIER '=' expression
    {
        DEBUG( LOG_DEBUG_SPAM, nullptr, "= %s\n", $4->get_node_name());
        $$ = ALLOCATOR->new_tracked<LSLDeclaration>(ALLOCATOR->new_tracked<LSLIdentifier>($1, $2, &@2), $4);
    }
    ;


forexpressionlist
    : /* empty */
    {
        $$ = nullptr;
    }
    | nextforexpressionlist
    {
        $$ = ALLOCATOR->new_tracked<LSLASTNodeList>($1);
    }
    ;

nextforexpressionlist
    : expression
    {
        $$ = $1;
    }
    | expression ',' nextforexpressionlist
    {
        if ( $1 ) {
            $1->add_next_sibling($3);
            $$ = $1;
        } else {
            $$ = $3;
        }
    }
    ;

funcexpressionlist
    : /* empty */
    {
        //$$ = ALLOCATOR->new_tracked<LSLExpression>(0);
        $$ = nullptr;
    }
    | nextfuncexpressionlist
    {
        $$ = $1;
    }
    ;

nextfuncexpressionlist
    : expression
    {
        $$ = $1;
    }
    | expression ',' nextfuncexpressionlist
    {
        if ( $1 ) {
            $1->add_next_sibling($3);
            $$ = $1;
        } else {
            $$ = $3;
        }
    }
    ;

listexpressionlist
    : /* empty */
    {
        //$$ = ALLOCATOR->new_tracked<LSLExpression>(0);
        //$$ = nullptr;
        $$ = nullptr;
    }
    | nextlistexpressionlist
    {
        $$ = $1;
    }
    ;

nextlistexpressionlist
    : expression
    {
        $$ = $1;
    }
    | expression ',' nextlistexpressionlist
    {
        if ($1) {
            $1->add_next_sibling($3);
            $$ = $1;
        } else {
            $$ = $3;
        }
    }
    ;

expression
    : unaryexpression
    {
        $$ = $1;
    }
    | lvalue '=' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '=', $3 );
    }
    | lvalue ADD_ASSIGN expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, ADD_ASSIGN, $3 );
    }
    | lvalue SUB_ASSIGN expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, SUB_ASSIGN, $3 );
    }
    | lvalue MUL_ASSIGN expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, MUL_ASSIGN, $3 );
    }
    | lvalue DIV_ASSIGN expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, DIV_ASSIGN, $3 );
    }
    | lvalue MOD_ASSIGN expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, MOD_ASSIGN, $3 );
    }
    | expression EQ expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, EQ, $3 );
    }
    | expression NEQ expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, NEQ, $3 );
    }
    | expression LEQ expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, LEQ, $3 );
    }
    | expression GEQ expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, GEQ, $3 );
    }
    | expression '<' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '<', $3 );
    }
    | expression '>' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '>', $3 );
    }
    | expression '+' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '+', $3 );
    }
    | expression '-' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '-', $3 );
    }
    | expression '*' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>( $1, '*', $3 );
    }
    | expression '/' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, '/',  $3  );
    }
    | expression '%' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, '%',  $3  );
    }
    | expression '&' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, '&',  $3  );
    }
    | expression '|' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, '|',  $3  );
    }
    | expression '^' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, '^',  $3  );
    }
    | expression BOOLEAN_AND expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, BOOLEAN_AND,  $3  );
    }
    | expression BOOLEAN_OR expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, BOOLEAN_OR,  $3  );
    }
    | expression SHIFT_LEFT expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, SHIFT_LEFT,  $3  );
    }
    | expression SHIFT_RIGHT expression
    {
        $$ = ALLOCATOR->new_tracked<LSLBinaryExpression>(  $1, SHIFT_RIGHT,  $3  );
    }
    ;

unaryexpression
    : '-' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>( $2, '-' );
    }
    | '!' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $2 , '!' );
    }
    | '~' expression
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $2 , '~' );
    }
    | INC_OP lvalue
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $2 , INC_PRE_OP );
    }
    | DEC_OP lvalue
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $2 , DEC_PRE_OP );
    }
    | typecast
    {
        $$ = $1;
    }
    | unarypostfixexpression
    {
        $$ = $1;
    }
    | '(' expression ')'
    {
        $$ = ALLOCATOR->new_tracked<LSLParenthesisExpression>($2);
    }
    ;

typecast
    : '(' typename ')' lvalue
    {
        $$ = ALLOCATOR->new_tracked<LSLTypecastExpression>($2, $4);
    }
    | '(' typename ')' unarypostfixexpression
    {
        $$ = ALLOCATOR->new_tracked<LSLTypecastExpression>($2, $4);
    }
    | '(' typename ')' '(' expression ')'
    {
        $$ = ALLOCATOR->new_tracked<LSLTypecastExpression>($2, $5);
    }
    ;

unarypostfixexpression
    : vector_initializer
    {
        DEBUG( LOG_DEBUG_SPAM, nullptr, "vector intializer..");
        $$ = $1;
    }
    | quaternion_initializer
    {
        $$ = $1;
    }
    | list_initializer
    {
        $$ = $1;
    }
    | lvalue
    {
        $$ = $1;
    }
    | lvalue INC_OP
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $1 , INC_POST_OP );
    }
    | lvalue DEC_OP
    {
        $$ = ALLOCATOR->new_tracked<LSLUnaryExpression>(  $1 , DEC_POST_OP );
    }
    | IDENTIFIER '(' funcexpressionlist ')'
    {
        if ( $3 != nullptr ) {
            $$ = ALLOCATOR->new_tracked<LSLFunctionExpression>( ALLOCATOR->new_tracked<LSLIdentifier>($1), $3 );
        } else {
            $$ = ALLOCATOR->new_tracked<LSLFunctionExpression>( ALLOCATOR->new_tracked<LSLIdentifier>($1) );
        }

    }
    | PRINT '(' expression ')'
    {
    /* This is effectively a no-op to most people in modern LSL, but we might need
       `expression`'s side-effects. Oh well. */
        $$ = ALLOCATOR->new_tracked<LSLPrintExpression>($3);
    }
    | constant
    {
        $$ = ALLOCATOR->new_tracked<LSLConstantExpression>($1);
    }
    ;

vector_initializer
    : '<' expression ',' expression ',' expression '>'    %prec INITIALIZER
    {
        $$ = ALLOCATOR->new_tracked<LSLVectorExpression>($2, $4, $6);
    }
    ;

quaternion_initializer
    : '<' expression ',' expression ',' expression ',' expression '>' %prec INITIALIZER
    {
        $$ = ALLOCATOR->new_tracked<LSLQuaternionExpression>($2, $4, $6, $8);
    }
    ;

list_initializer
    : '[' listexpressionlist ']' %prec INITIALIZER
    {
        $$ = ALLOCATOR->new_tracked<LSLListExpression>($2);
    }
    ;

lvalue
    : IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLLValueExpression>(ALLOCATOR->new_tracked<LSLIdentifier>($1));
    }
    | IDENTIFIER PERIOD IDENTIFIER
    {
        $$ = ALLOCATOR->new_tracked<LSLLValueExpression>(
            ALLOCATOR->new_tracked<LSLIdentifier>($1),
            ALLOCATOR->new_tracked<LSLIdentifier>($3)
        );
    }
    ;

%%

int yyerror( YYLTYPE *lloc, void *scanner, const char *message ) {
    tailslide_get_extra(scanner)->logger->error( lloc, E_SYNTAX_ERROR, message );
    tailslide_get_extra(scanner)->ast_sane = false;
    return 0;
}
