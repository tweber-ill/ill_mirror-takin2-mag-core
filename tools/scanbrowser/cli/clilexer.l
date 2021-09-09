/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 * @desc Originally forked from: https://github.com/t-weber/misc/tree/master/parser
 *
 * References:
 *	https://github.com/westes/flex/tree/master/examples/manual
 *	http://www.gnu.org/software/bison/manual/html_node/index.html
 *	http://git.savannah.gnu.org/cgit/bison.git/tree/examples
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

%option noyywrap
%option yyclass = "CliLexer"


%{
	#include "cliparser.h"
%}


sign     [-+]
integer  {sign}?[0-9]+
uinteger [0-9]+
real     {uinteger}(\.[0-9]*)?|\.[0-9]+


%%


[;\n] { return yy::CliParser::make_TOK_NEWLINE(); }
[ \t]		/* eps */

"(" { return yy::CliParser::make_TOK_BRACKET_OPEN(); }
")" { return yy::CliParser::make_TOK_BRACKET_CLOSE(); }
"[" { return yy::CliParser::make_TOK_SQBRACKET_OPEN(); }
"]" { return yy::CliParser::make_TOK_SQBRACKET_CLOSE(); }
"{" { return yy::CliParser::make_TOK_CURLBRACKET_OPEN(); }
"}" { return yy::CliParser::make_TOK_CURLBRACKET_CLOSE(); }
"+" { return yy::CliParser::make_TOK_PLUS(); }
"-" { return yy::CliParser::make_TOK_MINUS(); }
"*" { return yy::CliParser::make_TOK_MULT(); }
"/" { return yy::CliParser::make_TOK_DIV(); }
"%" { return yy::CliParser::make_TOK_MOD(); }
"^" { return yy::CliParser::make_TOK_POW(); }
"=" { return yy::CliParser::make_TOK_ASSIGN(); }
"." { return yy::CliParser::make_TOK_MEMBER_ACCESS(); }
"," { return yy::CliParser::make_TOK_COMMA(); }


{real}([eE]{integer})? {
	//std::cerr << "matched: " << yytext << std::endl;
	return yy::CliParser::make_TOK_REAL(str_to_real<t_real_cli>(yytext));
}

["'][^"'\n]*["'] {
	auto str = std::string(yytext+1, yytext+yyleng-1);
	return yy::CliParser::make_TOK_STRING(str);
}

[A-Za-z_][A-Za-z0-9_]* {
	return yy::CliParser::make_TOK_IDENT(yytext);
}


. {
	ctx.PrintError(std::string("Lexer error: Unknown token: \"") + yytext + std::string("\""));
} 


%%
