/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 */

%option noyywrap
%option yyclass = "CliLexer"


%{
	#include "cliparser.h"
%}

/*
integer [+-]?[0-9]+
real 	[+-]?(([0-9]+\.?[0-9]*)|(\.[0-9]+))
*/

integer [0-9]+
real 	(([0-9]+\.?[0-9]*)|(\.[0-9]+))


%%


{real}([eE]{integer})? {
	//std::cerr << "matched: " << yytext << std::endl;
	return yy::CliParser::make_TOK_REAL(str_to_real<t_real_cli>(yytext));
}

"\"".*"\"" {
	auto str = std::string(yytext+1, yytext+yyleng-1);
	return yy::CliParser::make_TOK_STRING(str);
}

[A-Za-z_]+[A-Za-z0-9]* {
	return yy::CliParser::make_TOK_IDENT(yytext);
}

"(" { return yy::CliParser::make_TOK_BRACKET_OPEN(); }
")" { return yy::CliParser::make_TOK_BRACKET_CLOSE(); }
"+" { return yy::CliParser::make_TOK_PLUS(); }
"-" { return yy::CliParser::make_TOK_MINUS(); }
"*" { return yy::CliParser::make_TOK_MULT(); }
"/" { return yy::CliParser::make_TOK_DIV(); }
"%" { return yy::CliParser::make_TOK_MOD(); }
"^" { return yy::CliParser::make_TOK_POW(); }
"=" { return yy::CliParser::make_TOK_ASSIGN(); }

"\n" { return yy::CliParser::make_TOK_NEWLINE(); }
[ \t]		/* eps */

. {
	context.PrintError(std::string("Lexer error: Unknown token: \"") + yytext + std::string("\""));
} 


%%
