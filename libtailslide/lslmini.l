%option bison-locations
%option bison-bridge
%option reentrant
%option noyywrap
%option nounput
%option always-interactive

%{
#include <stdlib.h>
#include "lslmini.hh"
#include "lslmini.tab.hh"

using namespace Tailslide;

#ifdef WIN32
#include <io.h>
#define isatty _isatty
#endif /* WIN32 */

#define LLOC_RESET()    yylloc->first_column = yylloc->first_line = yylloc->last_column = yylloc->last_line = 1;
#define LLOC_LINES(num) yylloc->last_column = 1; yylloc->last_line += (num);
#define LLOC_STEP()     yylloc->first_column = yylloc->last_column; yylloc->first_line = yylloc->last_line;
#define YY_USER_ACTION  yylloc->last_column += yyleng;
#define YY_USER_INIT    LLOC_RESET()

namespace Tailslide {
    char *parse_string(char *input);
}

%}


N			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{N}+
FS			(f|F)

 /* exclusive state to eat comments of any length without overflowing any buffers */
%x COMMENT
%x C_COMMENT

%%
%{
  /* HACK */
  yylloc = yylloc_param;
  LLOC_STEP();
%}
"//"                { BEGIN COMMENT; }
<COMMENT>"$[E"{N}{5}"]" {
    ErrorCode e = (ErrorCode) strtoul( yytext+3, NULL, 10 );
    LOG( LOG_INFO, yylloc, "Adding assertion for E%d.", (int)e );
    Logger::get()->add_assertion( yylloc->first_line, e );
}
<COMMENT>.          { /* eat comments */ }
<COMMENT>\n         { BEGIN 0; LLOC_LINES(1); LLOC_STEP(); } 

"/*"              { BEGIN C_COMMENT; }
<C_COMMENT>"*/"   { BEGIN 0; LLOC_STEP(); }
<C_COMMENT>\n     { LLOC_LINES(1); LLOC_STEP(); }
<C_COMMENT>.      { LLOC_STEP(); }

"integer"			{ return(INTEGER); }
"float"				{ return(FLOAT_TYPE); }
"string"			{ return(STRING); }
"key"				{ return(LLKEY); }
"vector"			{ return(VECTOR); }
"quaternion"		{ return(QUATERNION); }
"rotation"			{ return(QUATERNION); }
"list"				{ return(LIST); }

"default"			{ yylval->sval = gAllocationManager->alloc(strlen(yytext) + 1); strcpy(yylval->sval, yytext); return(STATE_DEFAULT); }
"state"				{ return(STATE); }
"event"				{ return(EVENT); }
"jump"				{ return(JUMP); }
"return"			{ return(RETURN); }
"if"				{ return(IF); }
"else"				{ return(ELSE); }
"for"				{ return(FOR); }
"do"				{ return(DO); }
"while"				{ return(WHILE); }
"print"				{ return(PRINT); }

"."						{ return(PERIOD); }


0[xX]{H}+			{ yylval->ival = strtoul(yytext, NULL, 0);  return(INTEGER_CONSTANT); }
{N}+                { yylval->ival = strtoul(yytext, NULL, 10); return(INTEGER_CONSTANT); }

{L}({L}|{N})*		{ yylval->sval = gAllocationManager->alloc(strlen(yytext) + 1); strcpy(yylval->sval, yytext); return(IDENTIFIER); }

{N}+{E}					{ yylval->fval = (F32)atof(yytext); return(FP_CONSTANT); }
{N}*"."{N}+({E})?{FS}?	{ yylval->fval = (F32)atof(yytext); return(FP_CONSTANT); }
{N}+"."{N}*({E})?{FS}?	{ yylval->fval = (F32)atof(yytext); return(FP_CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ yylval->sval = Tailslide::parse_string(yytext); return(STRING_CONSTANT); }

"++"				{ return(INC_OP); }
"--"				{ return(DEC_OP); }
"+="				{ return(ADD_ASSIGN); }
"-="				{ return(SUB_ASSIGN); }
"*="				{ return(MUL_ASSIGN); }
"/="				{ return(DIV_ASSIGN); }
"%="				{ return(MOD_ASSIGN); }
";"					{ return(';'); }
"{"					{ return('{'); }
"}"					{ return('}'); }
","					{ return(','); }
"="					{ return('='); }
"("					{ return('('); }
")"					{ return(')'); }
"-"					{ return('-'); }
"+"					{ return('+'); }
"*"					{ return('*'); }
"/"					{ return('/'); }
"%"					{ return('%'); }
"@"					{ return('@'); }
":"					{ return(':'); }
">"					{ return('>'); }
"<"					{ return('<'); }
"]"					{ return(']'); }
"["					{ return('['); }
"=="				{ return(EQ);  }
"!="				{ return(NEQ);  }
">="				{ return(GEQ);  }
"<="				{ return(LEQ);  }
"&"					{ return('&');  }
"|"					{ return('|');  }
"^"					{ return('^');  }
"~"					{ return('~');  }
"!"					{ return('!');  }
"&&"				{ return(BOOLEAN_AND);	}
"||"				{ return(BOOLEAN_OR);	}
"<<"				{ return(SHIFT_LEFT);  }
">>"				{ return(SHIFT_RIGHT); }


\n                  { LLOC_LINES(1); LLOC_STEP(); } 
.					{ LLOC_STEP(); /* ignore bad characters */ }

%%

