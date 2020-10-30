/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 */

%define parser_class_name { CliParser }
//%define api.parser.class { CliParser }
%define api.value.type variant
%define api.token.constructor
//%error-verbose
%define parse.error verbose

%code requires { #include "cliparser_types.h" }
%code { #include "cliparser.h" }
%param { CliParserContext &ctx }


// terminals
%token<t_real_cli> TOK_REAL
%token<std::string> TOK_STRING TOK_IDENT
%token TOK_BRACKET_OPEN TOK_BRACKET_CLOSE
%token TOK_SQBRACKET_OPEN TOK_SQBRACKET_CLOSE
%token TOK_CURLBRACKET_OPEN TOK_CURLBRACKET_CLOSE
%token TOK_PLUS TOK_MINUS TOK_MULT TOK_DIV TOK_MOD TOK_POW
%token TOK_ASSIGN
%token TOK_MEMBER_ACCESS
%token TOK_COMMA
%token TOK_NEWLINE
//%token TOK_EOF


// non-terminals
%type<std::shared_ptr<CliAST>> command commands
%type<std::shared_ptr<CliAST>> expression expressions
%type<std::shared_ptr<CliAST>> ident


// operator precedence
%right TOK_ASSIGN
%left TOK_PLUS TOK_MINUS
%left TOK_MULT TOK_DIV TOK_MOD
%right PREC_UNARY_PLUSMINUS
%nonassoc TOK_POW
%nonassoc TOK_MEMBER_ACCESS
%left TOK_SQBRACKET_OPEN TOK_CURLBRACKET_OPEN


%%

commands
	: commands command
	    { ctx.AddAST($2); }
	| /* eps */
	    { $$ = nullptr; }
	;

command
	: expression TOK_NEWLINE
	    { $$ = $1; }
	| TOK_NEWLINE
	    { $$ = nullptr; }
	;

ident
	: TOK_IDENT
	    { $$ = std::make_shared<CliASTIdent>($1); }
	;

expressions
	: expressions TOK_COMMA expression
		{ $$ = std::make_shared<CliASTExprList>($1, $3); }
	| expression
		{ $$ = $1; }
	| /* eps */
	    { $$ = nullptr; }
	;

expression
	: ident TOK_ASSIGN expression
		{ $$ = std::make_shared<CliASTAssign>($1, $3); }
	| ident TOK_BRACKET_OPEN expressions TOK_BRACKET_CLOSE
		{ $$ = std::make_shared<CliASTCall>($1, $3); }
	| expression TOK_MEMBER_ACCESS ident TOK_BRACKET_OPEN expressions TOK_BRACKET_CLOSE
		{ $$ = std::make_shared<CliASTCall>($3, std::make_shared<CliASTExprList>($1, $5)); }

	| TOK_SQBRACKET_OPEN expressions TOK_SQBRACKET_CLOSE
		{ $$ = std::make_shared<CliASTArray>($2); }
	| expression TOK_SQBRACKET_OPEN expression TOK_SQBRACKET_CLOSE
		{ $$ = std::make_shared<CliASTArrayAccess>($1, $3); }

	| TOK_BRACKET_OPEN expression TOK_BRACKET_CLOSE
		{ $$ = $2; }
	| TOK_PLUS expression %prec PREC_UNARY_PLUSMINUS
		{ $$ = $2; }
	| TOK_MINUS expression %prec PREC_UNARY_PLUSMINUS
		{ $$ = std::make_shared<CliASTMinus>(nullptr, $2); }

	| expression TOK_PLUS expression
		{ $$ = std::make_shared<CliASTPlus>($1, $3); }
	| expression TOK_MINUS expression
		{ $$ = std::make_shared<CliASTMinus>($1, $3); }
	| expression TOK_MULT expression
		{ $$ = std::make_shared<CliASTMult>($1, $3); }
	| expression TOK_DIV expression
		{ $$ = std::make_shared<CliASTDiv>($1, $3); }
	| expression TOK_MOD expression
		{ $$ = std::make_shared<CliASTMod>($1, $3); }
	| expression TOK_POW expression
		{ $$ = std::make_shared<CliASTPow>($1, $3); }

	| ident
		{ $$ = $1; }
	| TOK_REAL
		{ $$ = std::make_shared<CliASTReal>($1); }
	| TOK_STRING
		{ $$ = std::make_shared<CliASTString>($1); }
	;
%%
