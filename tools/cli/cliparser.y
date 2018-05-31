/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 */

%define parser_class_name { CliParser }
%define api.value.type variant
%define api.token.constructor
%error-verbose

%code requires { #include "cliparser_types.h" }
%code { #include "cliparser.h" }
%param { CliParserContext &context }


%token<t_real_cli> TOK_REAL
%token<std::string> TOK_STRING TOK_IDENT
%token TOK_BRACKET_OPEN TOK_BRACKET_CLOSE
%token TOK_PLUS TOK_MINUS TOK_MULT TOK_DIV TOK_MOD TOK_POW
%token TOK_ASSIGN
%token TOK_NEWLINE
//%token TOK_EOF

%type<std::shared_ptr<CliAST>> expression ident
%type<std::shared_ptr<CliAST>> command commands

%right TOK_ASSIGN
%left TOK_PLUS TOK_MINUS
%left TOK_MULT TOK_DIV TOK_MOD
%right PREC_UNARY_PLUSMINUS
%nonassoc TOK_POW

%%

commands
	: commands command
	    { $$ = $2; }
	| /* eps */
	    { $$ = nullptr; }
	;

command
	: expression TOK_NEWLINE
	    { $$ = $1; $$->Print(); }
	| TOK_NEWLINE
	    { $$ = nullptr; }
	;

ident
	: TOK_IDENT
	    { $$ = std::make_shared<CliASTIdent>($1); }
	;

expression
	: ident TOK_ASSIGN expression
		{ $$ = std::make_shared<CliASTAssign>($1, $3); }
	| ident TOK_BRACKET_OPEN TOK_BRACKET_CLOSE
		{ $$ = std::make_shared<CliASTCall>(); }

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
