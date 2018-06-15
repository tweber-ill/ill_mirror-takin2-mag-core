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

#include "tools/in20/data.h"


using t_real = t_real_cli;

class CliAST;
class CliParserContext;



// ----------------------------------------------------------------------------
// Lexer
// ----------------------------------------------------------------------------

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
	std::vector<std::shared_ptr<CliAST>> m_asts;
	std::vector<std::string> m_errors;

public:
	CliLexer& GetLexer() { return m_lex; }

	void PrintError(const std::string &err);
	const std::vector<std::string>& GetErrors() const { return m_errors; }
	void ClearErrors() { m_errors.clear(); }

	void SetLexerInput(std::istream &istr);

	void AddAST(std::shared_ptr<CliAST> ast) { m_asts.push_back(ast); }
	void ClearASTs() { m_asts.clear(); }
	const std::vector<std::shared_ptr<CliAST>>& GetASTs() const { return m_asts; }
};

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// Symbols
// ----------------------------------------------------------------------------

enum class SymbolType
{
	REAL,
	STRING,
	DATASET
};


class Symbol
{
public:
	virtual ~Symbol() {}

	virtual SymbolType GetType() const = 0;
	virtual std::shared_ptr<Symbol> copy() const = 0;

	static std::shared_ptr<Symbol> uminus(const Symbol &sym2);
	static std::shared_ptr<Symbol> add(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> sub(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> mul(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> div(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> mod(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> pow(const Symbol &sym1, const Symbol &sym2);
};


class SymbolReal : public Symbol
{
private:
	t_real m_val = 0;

public:
	SymbolReal() = default;
	SymbolReal(t_real val) : m_val(val) {}
	virtual ~SymbolReal() {}

	virtual SymbolType GetType() const override { return SymbolType::REAL; }
	t_real GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolReal>(m_val); }
};


class SymbolString : public Symbol
{
private:
	std::string m_val;

public:
	SymbolString() = default;
	SymbolString(const std::string& val) : m_val(val) {}
	virtual ~SymbolString() {}

	virtual SymbolType GetType() const override { return SymbolType::STRING; }
	const std::string& GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolString>(m_val); }
};


class SymbolDataset : public Symbol
{
private:
	Dataset m_val;

public:
	SymbolDataset() = default;
	SymbolDataset(const Dataset& val) : m_val(val) {}
	virtual ~SymbolDataset() {}

	virtual SymbolType GetType() const override { return SymbolType::DATASET; }
	const Dataset& GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolDataset>(m_val); }
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
	virtual std::shared_ptr<Symbol> Eval() const = 0;
};


class CliASTReal : public CliAST
{
protected:
	t_real_cli m_val = t_real_cli(0);

public:
	CliASTReal(t_real_cli val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTString : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTString(const std::string& val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTIdent : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTIdent(const std::string& val) : m_val(val) { }

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTAssign : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTPlus : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTMinus : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTMult : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTDiv : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTMod : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTPow : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};


class CliASTCall : public CliAST
{
public:
	using CliAST::CliAST;

	virtual void Print(int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval() const override;
};

// ----------------------------------------------------------------------------



#undef YY_DECL
#define YY_DECL yy::CliParser::symbol_type CliLexer::yylex(CliParserContext &context)
extern yy::CliParser::symbol_type yylex(CliParserContext &context);

#define yyterminate() return yy::CliParser::token::yytokentype(YY_NULL);


#endif
