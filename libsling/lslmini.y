%{
    #include "lslmini.hh"
    #include "logger.hh"
    #include <stdio.h>
    #include <string.h>

    //int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param);
    extern int yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param , void *yyscanner);

%}
%{
    using namespace Sling;
    thread_local LLScriptScript *Sling::gScript;
    thread_local bool Sling::gFatalError;
    int yyerror( YYLTYPE*, void *, const char * );
    #define MAKEID(type,id,pos) gAllocationManager->new_tracked<LLScriptIdentifier>(TYPE(type), (id), &(pos))
    // slightly higher so we can still have assert comments that check for stack depth
    #define YYMAXDEPTH 10020
    #define YYINITDEPTH 10020
    #define LSLINT_STACK_OVERFLOW_AT 10000
    inline int _yylex( YYSTYPE * yylval, YYLTYPE *yylloc, void *yyscanner, int stack ) {
        if ( stack == LSLINT_STACK_OVERFLOW_AT ) {
            ERROR( yylloc, E_PARSER_STACK_DEPTH );
            return 0;
        }
        return yylex( yylval, yylloc, yyscanner );
    }
    #define yylex(a,b,c) _yylex(a, b, c,  (int)(yyssp - yyss))
        

    // Same as bison's default, but update global position so we don't have
    // to pass it in every time we make a branch
    # define YYLLOC_DEFAULT(Current, Rhs, N)                \
        ((Current).first_line   = (Rhs)[1].first_line,       \
         (Current).first_column = (Rhs)[1].first_column,     \
         (Current).last_line    = (Rhs)[N].last_line,        \
         (Current).last_column  = (Rhs)[N].last_column,      \
         LLASTNode::set_glloc(&(Current)))

%}

%define parse.error verbose
%locations
%define api.pure
%parse-param { void *scanner }
%lex-param { void *scanner }

%code requires {
    namespace Sling {
        typedef int   S32;
        typedef float F32;
        class LLScriptType;
        class LLScriptConstant;
        class LLScriptIdentifier;
        class LLScriptSimpleAssignable;
        class LLScriptGlobalVariable;
        class LLScriptEvent;
        class LLScriptEventHandler;
        class LLScriptExpression;
        class LLScriptStatement;
        class LLScriptGlobalFunction;
        class LLScriptFunctionDec;
        class LLScriptEventDec;
        class LLScriptForExpressionList;
        class LLScriptState;
        class LLScriptGlobalStorage;
        class LLScriptScript;
    }
}

%union
{
	Sling::S32								ival;
	Sling::F32								fval;
	char							*sval;
	class Sling::LLScriptType				*type;
	class Sling::LLScriptConstant			*constant;
	class Sling::LLScriptIdentifier		*identifier;
	class Sling::LLScriptSimpleAssignable	*assignable;
	class Sling::LLScriptGlobalVariable	*global;
	class Sling::LLScriptEvent				*event;
	class Sling::LLScriptEventHandler		*handler;
	class Sling::LLScriptExpression		*expression;
	class Sling::LLScriptStatement			*statement;
	class Sling::LLScriptGlobalFunction	*global_funcs;
	class Sling::LLScriptFunctionDec		*global_decl;
	class Sling::LLScriptEventDec		*global_event_decl;
    class Sling::LLScriptForExpressionList *for_expr;
	class Sling::LLScriptState				*state;
	class Sling::LLScriptGlobalStorage		*global_store;
	class Sling::LLScriptScript			*script;
};


%token					INTEGER
%token					FLOAT_TYPE
%token					STRING
%token					LLKEY
%token					VECTOR
%token					QUATERNION
%token					LIST

%token					STATE
%token					EVENT
%token					JUMP
%token					RETURN

%token <sval>			IDENTIFIER
%token <sval>			STATE_DEFAULT

%token <ival>			INTEGER_CONSTANT
%token <ival>			INTEGER_TRUE
%token <ival>			INTEGER_FALSE

%token <fval>			FP_CONSTANT

%token <sval>			STRING_CONSTANT

%token					INC_OP
%token					DEC_OP
%token					INC_PRE_OP
%token					DEC_PRE_OP
%token					INC_POST_OP
%token					DEC_POST_OP
%token					ADD_ASSIGN
%token					SUB_ASSIGN
%token					MUL_ASSIGN
%token					DIV_ASSIGN
%token					MOD_ASSIGN

%token					EQ
%token					NEQ
%token					GEQ
%token					LEQ

%token					BOOLEAN_AND
%token					BOOLEAN_OR

%token					SHIFT_LEFT
%token					SHIFT_RIGHT

%token					IF
%token					ELSE
%token					FOR
%token					DO
%token					WHILE

%token					PRINT

%token					PERIOD

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%type <script>			lscript_program
%type <global_store>	globals
%type <global_store>	global
%type <global>			global_variable
%type <assignable>		simple_assignable
%type <assignable>		simple_assignable_no_list
%type <constant>		constant
%type <assignable>		special_constant
%type <assignable>		vector_constant
%type <assignable>		quaternion_constant
%type <assignable>		list_constant
%type <assignable>		list_entries
%type <assignable>		list_entry
%type <type>			typename
%type <global_funcs>	global_function
%type <global_decl>		function_parameters
%type <global_decl>		function_parameter
%type <global_event_decl>		event_parameters
%type <global_event_decl>		event_parameter
%type <state>			states
%type <state>			other_states
%type <state>			default
%type <state>			state
%type <handler>			state_body
%type <handler>			event
%type <statement>		compound_statement
%type <statement>		statement
%type <statement>		statements
%type <statement>		declaration
%type <statement>		';'
%type <statement>		'@'
%type <for_expr>      forexpressions
%type <for_expr>  	  forexpression
%type <expression>    nextfuncexpressionlist
%type <expression>		funcexpressionlist
%type <expression>		nextlistexpressionlist
%type <expression>		listexpressionlist
%type <expression>		unarypostfixexpression
%type <expression>		vector_initializer
%type <expression>		quaternion_initializer
%type <expression>		list_initializer
%type <expression>		lvalue
%type <expression>		'-'
%type <expression>		'!'
%type <expression>		'~'
%type <expression>		'='
%type <expression>		'<'
%type <expression>		'>'
%type <expression>		'+'
%type <expression>		'*'
%type <expression>		'/'
%type <expression>		'%'
%type <expression>		'&'
%type <expression>		'|'
%type <expression>		'^'
%type <expression>		ADD_ASSIGN
%type <expression>		SUB_ASSIGN
%type <expression>		MUL_ASSIGN
%type <expression>		DIV_ASSIGN
%type <expression>		MOD_ASSIGN
%type <expression>		EQ
%type <expression>		NEQ
%type <expression>		LEQ
%type <expression>		GEQ
%type <expression>		BOOLEAN_AND
%type <expression>		BOOLEAN_OR
%type <expression>		SHIFT_LEFT
%type <expression>		SHIFT_RIGHT
%type <expression>		INC_OP
%type <expression>		DEC_OP
%type <expression>		'('
%type <expression>		')'
%type <expression>		PRINT
%type <identifier>		name_type
%type <expression>		expression
%type <expression>		unaryexpression
%type <expression>		typecast

%right '=' MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%left 	BOOLEAN_AND BOOLEAN_OR
%left	'|'
%left	'^'
%left	'&'
%left	EQ NEQ
%left	'<' LEQ '>' GEQ
%left	SHIFT_LEFT SHIFT_RIGHT
%left 	'+' '-'
%left	'*' '/' '%'
%right	'!' '~' INC_OP DEC_OP
%nonassoc INITIALIZER

%%

lscript_program
	: globals states		
	{
    gScript = gAllocationManager->new_tracked<LLScriptScript>($1, $2);
	}
	| states
	{
    gScript = gAllocationManager->new_tracked<LLScriptScript>(nullptr, $1);
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
    $$ = gAllocationManager->new_tracked<LLScriptGlobalStorage>($1, nullptr);
	}
	| global_function
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalStorage>(nullptr, $1);
	}
	;

name_type
	: typename IDENTIFIER
	{
    $$ = gAllocationManager->new_tracked<LLScriptIdentifier>($1, $2, &@2);
	}
	;

global_variable
	: name_type ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalVariable>($1, nullptr);
	}
	| name_type '=' simple_assignable ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalVariable>($1, $3);
	}
    | name_type '=' expression ';'
    {
    ERROR(&@3, E_GLOBAL_INITIALIZER_NOT_CONSTANT);
    $$ = nullptr;
    }
    | name_type '=' error ';'
    {
    $$ = nullptr;
    }
	;

simple_assignable
	: simple_assignable_no_list
	{
    $$ = $1;
	}
	| list_constant
	{
    $$ = $1;
	}
	;

simple_assignable_no_list
	: IDENTIFIER
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(
        gAllocationManager->new_tracked<LLScriptIdentifier>($1)
    );
	}
	| constant
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>($1);
	}
	| special_constant
	{
    $$ = $1; //gAllocationManager->new_tracked<LLScriptSimpleAssignable>($1);
	}
	;

constant
  : '-' INTEGER_CONSTANT
  {
    $$ = gAllocationManager->new_tracked<LLScriptIntegerConstant>(-$2);
  }
  | INTEGER_CONSTANT
	{
    $$ = gAllocationManager->new_tracked<LLScriptIntegerConstant>($1);
	}
    | '-' FP_CONSTANT
    {
    $$ = gAllocationManager->new_tracked<LLScriptFloatConstant>(-$2);
    }
	| FP_CONSTANT
	{
    $$ = gAllocationManager->new_tracked<LLScriptFloatConstant>($1);
	}
	| STRING_CONSTANT
	{
    $$ = gAllocationManager->new_tracked<LLScriptStringConstant>($1);
	}
	;

special_constant
	: vector_constant
	{
    $$ = $1;
	}
	| quaternion_constant
	{
    $$ = $1;
	}
	;

vector_constant
	: '<' simple_assignable ',' simple_assignable ',' simple_assignable '>'
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(gAllocationManager->new_tracked<LLScriptVectorConstant>($2, $4, $6));
	}
	;

quaternion_constant
	: '<' simple_assignable ',' simple_assignable ',' simple_assignable ',' simple_assignable '>'
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(gAllocationManager->new_tracked<LLScriptQuaternionConstant>($2, $4, $6, $8));
	}
	;

list_constant
	: '[' list_entries ']'
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(gAllocationManager->new_tracked<LLScriptListConstant>($2));
	}
	| '[' ']'
	{
    $$ = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(gAllocationManager->new_tracked<LLScriptListConstant>((LLScriptSimpleAssignable*)nullptr));
	}
	;

list_entries
	: list_entry
	{
    $$ = $1;
	}
	| list_entry ',' list_entries
	{
    if ( $1 ) {
        $1->add_next_sibling($3);
        $$ = $1;
    } else {
        $$ = $3;
    }
	}
	;

list_entry
	: simple_assignable_no_list
	{
    $$ = $1;
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
    $$ = gAllocationManager->new_tracked<LLScriptGlobalFunction>( MAKEID(LST_NULL, $1, @1), nullptr, $4 );
	}
	| name_type '(' ')' compound_statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalFunction>( $1, nullptr, $4 );
	}
	| IDENTIFIER '(' function_parameters ')' compound_statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalFunction>( MAKEID(LST_NULL, $1, @1), $3, $5 );
	}
	| name_type '(' function_parameters ')' compound_statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptGlobalFunction>( $1, $3, $5 );
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
    $$ = gAllocationManager->new_tracked<LLScriptFunctionDec>( gAllocationManager->new_tracked<LLScriptIdentifier>($1, $2, &@2) );
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
    $$ = gAllocationManager->new_tracked<LLScriptEventDec>( gAllocationManager->new_tracked<LLScriptIdentifier>($1, $2, &@2) );
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
    $$ = gAllocationManager->new_tracked<LLScriptState>( nullptr, $3 );
	}
    | STATE_DEFAULT '{' '}'
    {
    ERROR( &@1, E_NO_EVENT_HANDLERS );
    $$ = gAllocationManager->new_tracked<LLScriptState>( nullptr, nullptr );
    }
	;

state
	: STATE IDENTIFIER '{' state_body '}'
	{
    $$ = gAllocationManager->new_tracked<LLScriptState>( MAKEID(LST_NULL, $2, @2), $4 );
	}
    | STATE IDENTIFIER '{' '}'
    {
    ERROR( &@1, E_NO_EVENT_HANDLERS );
    $$ = gAllocationManager->new_tracked<LLScriptState>( nullptr, nullptr );
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
    $$ = gAllocationManager->new_tracked<LLScriptEventHandler>(MAKEID(LST_NULL, $1, @1), nullptr, $4);
	}
	| IDENTIFIER '(' event_parameters ')' compound_statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptEventHandler>(MAKEID(LST_NULL, $1, @1), $3, $5);
	}
   ;

compound_statement
	: '{' '}'
	{
    $$ = gAllocationManager->new_tracked<LLScriptCompoundStatement>(nullptr);
	}
	| '{' statements '}'
	{
    $$ = gAllocationManager->new_tracked<LLScriptCompoundStatement>($2);
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
    $$ = gAllocationManager->new_tracked<LLScriptStatement>(nullptr);
	}
	| STATE IDENTIFIER ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptStateStatement>(MAKEID(LST_NULL, $2, @2));
	}
	| STATE STATE_DEFAULT ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptStateStatement>();
	}
	| JUMP IDENTIFIER ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptJumpStatement>(MAKEID(LST_NULL, $2, @2));
	}
	| '@' IDENTIFIER ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptLabel>(MAKEID(LST_NULL, $2, @2));
	}
	| RETURN expression ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptReturnStatement>($2);
	}
	| RETURN ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptReturnStatement>(nullptr);
	}
	| expression ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptStatement>($1);
	}
	| declaration ';'
	{
    $$ = $1;
	}
	| compound_statement
	{
    $$ = $1;
	}
	| IF '(' expression ')' statement	%prec LOWER_THAN_ELSE
	{
    $$ = gAllocationManager->new_tracked<LLScriptIfStatement>($3, $5, nullptr);
    $5->set_declaration_allowed(false);
	}
	| IF '(' expression ')' statement ELSE statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptIfStatement>($3, $5, $7);
    $5->set_declaration_allowed(false);
    $7->set_declaration_allowed(false);
	}
  | FOR '(' forexpressions ';' expression ';' forexpressions ')' statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptForStatement>($3, $5, $7, $9);
    $9->set_declaration_allowed(false);
	}
	| DO statement WHILE '(' expression ')' ';'
	{
    $$ = gAllocationManager->new_tracked<LLScriptDoStatement>($2, $5);
    $2->set_declaration_allowed(false);
	}
	| WHILE '(' expression ')' statement
	{
    $$ = gAllocationManager->new_tracked<LLScriptWhileStatement>($3, $5);
    $5->set_declaration_allowed(false);
	}
    | error ';'
    {
    $$ = gAllocationManager->new_tracked<LLScriptStatement>(nullptr);
    }
	;

declaration
	: typename IDENTIFIER
	{
    $$ = gAllocationManager->new_tracked<LLScriptDeclaration>(gAllocationManager->new_tracked<LLScriptIdentifier>($1, $2, &@2), nullptr);
	}
	| typename IDENTIFIER '=' expression
	{
    DEBUG( LOG_DEBUG_SPAM, nullptr, "= %s\n", $4->get_node_name());
    $$ = gAllocationManager->new_tracked<LLScriptDeclaration>(gAllocationManager->new_tracked<LLScriptIdentifier>($1, $2, &@2), $4);
	}
	;

forexpressions
  : /* empty */
  {
    //$$ = gAllocationManager->new_tracked<LLScriptExpression>(0);
    $$ = nullptr;
  }
  | forexpression
  {
    $$ = $1;
  }
  | forexpression ',' forexpressions
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

forexpression
  : expression
  {
    $$ = gAllocationManager->new_tracked<LLScriptForExpressionList>( $1 );
  }
  ;

funcexpressionlist
	: /* empty */
	{
    //$$ = gAllocationManager->new_tracked<LLScriptExpression>(0);
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
    //$$ = gAllocationManager->new_tracked<LLScriptExpression>(0);
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
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '=', $3 );
	}
	| lvalue ADD_ASSIGN expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, ADD_ASSIGN, $3 );
	}
	| lvalue SUB_ASSIGN expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, SUB_ASSIGN, $3 );
	}
	| lvalue MUL_ASSIGN expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, MUL_ASSIGN, $3 );
	}
	| lvalue DIV_ASSIGN expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, DIV_ASSIGN, $3 );
	}
	| lvalue MOD_ASSIGN expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, MOD_ASSIGN, $3 );
	}
	| expression EQ expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, EQ, $3 );
	}
	| expression NEQ expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, NEQ, $3 );
	}
	| expression LEQ expression
  {
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, LEQ, $3 );
	}
	| expression GEQ expression
  {
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, GEQ, $3 );
	}
	| expression '<' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '<', $3 );
	}
	| expression '>' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '>', $3 );
	}
	| expression '+' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '+', $3 );
	}
	| expression '-' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '-', $3 );
	}
	| expression '*' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>( $1, '*', $3 );
	}
	| expression '/' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, '/',  $3  );
	}
	| expression '%' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, '%',  $3  );
	}
	| expression '&' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, '&',  $3  );
	}
	| expression '|' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, '|',  $3  );
	}
	| expression '^' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, '^',  $3  );
	}
	| expression BOOLEAN_AND expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, BOOLEAN_AND,  $3  );
	}
	| expression BOOLEAN_OR expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, BOOLEAN_OR,  $3  );
	}
	| expression SHIFT_LEFT expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, SHIFT_LEFT,  $3  );
	}
	| expression SHIFT_RIGHT expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptBinaryExpression>(  $1, SHIFT_RIGHT,  $3  );
	}
     ;

unaryexpression
	: '-' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>( $2, '-' );
	}
	| '!' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $2 , '!' );
	}
	| '~' expression
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $2 , '~' );
	}
	| INC_OP lvalue
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $2 , INC_PRE_OP );
	}
	| DEC_OP lvalue
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $2 , DEC_PRE_OP );
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
    $$ = gAllocationManager->new_tracked<LLScriptParenthesisExpression>($2);
	}
    ;

typecast
	: '(' typename ')' lvalue
	{
    $$ = gAllocationManager->new_tracked<LLScriptTypecastExpression>($2, $4);
	}
	| '(' typename ')' constant
	{
    $$ = gAllocationManager->new_tracked<LLScriptTypecastExpression>(
        $2,
        gAllocationManager->new_tracked<LLScriptConstantExpression>($4)
    );
	}
	| '(' typename ')' unarypostfixexpression
	{
    $$ = gAllocationManager->new_tracked<LLScriptTypecastExpression>($2, $4);
	}
	| '(' typename ')' '(' expression ')'
	{
    $$ = gAllocationManager->new_tracked<LLScriptTypecastExpression>($2, $5);
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
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $1 , INC_POST_OP );
	}
	| lvalue DEC_OP
	{
    $$ = gAllocationManager->new_tracked<LLScriptUnaryExpression>(  $1 , DEC_POST_OP );
	}
	| IDENTIFIER '(' funcexpressionlist ')'
	{
    if ( $3 != nullptr ) {
      $$ = gAllocationManager->new_tracked<LLScriptFunctionExpression>( gAllocationManager->new_tracked<LLScriptIdentifier>($1), $3 );
    } else {
      $$ = gAllocationManager->new_tracked<LLScriptFunctionExpression>( gAllocationManager->new_tracked<LLScriptIdentifier>($1) );
    }

	}
	| PRINT '(' expression ')'
	{
    /* This is essentially a no-op in modern LSL, bug we might need
       `expression`'s side-effects. Oh well. */
    $$ = gAllocationManager->new_tracked<LLScriptPrintExpression>($3);
	}
	| constant
	{
    $$ = gAllocationManager->new_tracked<LLScriptConstantExpression>($1);
	}
	;

vector_initializer
	: '<' expression ',' expression ',' expression '>'	%prec INITIALIZER
	{
    $$ = gAllocationManager->new_tracked<LLScriptVectorExpression>($2, $4, $6);
	}
	;

quaternion_initializer
	: '<' expression ',' expression ',' expression ',' expression '>' %prec INITIALIZER
	{
    $$ = gAllocationManager->new_tracked<LLScriptQuaternionExpression>($2, $4, $6, $8);
	}
	;

list_initializer
	: '[' listexpressionlist ']' %prec INITIALIZER
	{
    $$ = gAllocationManager->new_tracked<LLScriptListExpression>($2);
	}
	;

lvalue
	: IDENTIFIER
	{
    $$ = gAllocationManager->new_tracked<LLScriptLValueExpression>(gAllocationManager->new_tracked<LLScriptIdentifier>($1));
	}
	| IDENTIFIER PERIOD IDENTIFIER
	{
    $$ = gAllocationManager->new_tracked<LLScriptLValueExpression>(
        gAllocationManager->new_tracked<LLScriptIdentifier>($1),
        gAllocationManager->new_tracked<LLScriptIdentifier>($3)
    );
	}
	;
		
%%

int yyerror( YYLTYPE *lloc, void *scanner, const char *message ) {
    ERROR( lloc, E_SYNTAX_ERROR, message );
    gFatalError = true;
    return 0;
}
