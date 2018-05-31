/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 */

#ifndef __CLI_PARSER_H__
#define __CLI_PARSER_H__

#include <sstream>
#include <string>
#include <vector>
#include <memory>

#undef yyFlexLexer
#include <FlexLexer.h>
#include "cliparser_types.h"
#include "cliparser_impl.h"



// ----------------------------------------------------------------------------
// Lexer
// ----------------------------------------------------------------------------

class CliParserContext;

class CliLexer : public yyFlexLexer
{
private:
	CliParserContext *m_pContext = nullptr;

protected:
	virtual void LexerError(const char *err) override;

public:
	CliLexer(CliParserContext *ctx = nullptr);
	virtual yy::CliParser::symbol_type yylex(CliParserContext &context);
};

template<class t_real> t_real str_to_real(const std::string& str);

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// Parser
// ----------------------------------------------------------------------------

class CliParserContext
{
private:
	CliLexer m_lex;

	std::vector<std::string> m_errors;

public:
	CliLexer& GetLexer() { return m_lex; }

	void PrintError(const std::string &err);
	const std::vector<std::string>& GetErrors() const { return m_errors; }
	void ClearErrors() { m_errors.clear(); }

	void SetLexerInput(std::istream &istr);
};

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// AST
// ----------------------------------------------------------------------------

class CliAST
{
protected:
	std::shared_ptr<CliAST> m_left;
	std::shared_ptr<CliAST> m_right;

public:
	CliAST(std::shared_ptr<CliAST> left=nullptr, std::shared_ptr<CliAST> right=nullptr) : m_left(left), m_right(right) {}

	void SetLeft(std::shared_ptr<CliAST> left) { m_left = left; }
	void SetRight(std::shared_ptr<CliAST> right) { m_right = right; }

	virtual void Print(int indent = 0) const;
};


class CliASTReal : public CliAST
{
protected:
	t_real_cli m_val = t_real_cli(0);

public:
	CliASTReal(t_real_cli val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
};


class CliASTString : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTString(const std::string& val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
};


class CliASTIdent : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTIdent(const std::string& val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
};


class CliASTAssign : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTPlus : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTMinus : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTMult : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTDiv : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTMod : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTPow : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};


class CliASTCall : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
};

// ----------------------------------------------------------------------------



#undef YY_DECL
#define YY_DECL yy::CliParser::symbol_type CliLexer::yylex(CliParserContext &context)
extern yy::CliParser::symbol_type yylex(CliParserContext &context);

#define yyterminate() return yy::CliParser::token::yytokentype(YY_NULL);


#endif
