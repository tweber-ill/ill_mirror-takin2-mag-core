/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
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

#ifndef __CLI_PARSER_H__
#define __CLI_PARSER_H__

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include <boost/signals2/signal.hpp>

#undef yyFlexLexer
#include <FlexLexer.h>
#include "cliparser_types.h"
#include "cliparser_impl.h"

#include "../data.h"


class CliAST;
class CliParserContext;
class Symbol;



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
	virtual yy::CliParser::symbol_type yylex(CliParserContext &ctx);
};

template<class t_real_cli> t_real_cli str_to_real(const std::string& str);

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

	// symbol tables and update signal
	std::map<std::string, std::shared_ptr<Symbol>> *m_workspace = nullptr;
	boost::signals2::signal<void(const std::string&)> m_WorkspaceUpdated;

public:
	CliLexer& GetLexer() { return m_lex; }

	void PrintErrorString(const std::string &err);

	template<typename ...T> void PrintError(T&&... msgs)
	{
		std::ostringstream ostr;
		(ostr << ... << std::forward<T>(msgs));
		PrintErrorString(ostr.str());
	}

	const std::vector<std::string>& GetErrors() const { return m_errors; }
	void ClearErrors() { m_errors.clear(); }

	void SetLexerInput(std::istream &istr);

	void AddAST(std::shared_ptr<CliAST> ast) { m_asts.push_back(ast); }
	void ClearASTs() { m_asts.clear(); }
	const std::vector<std::shared_ptr<CliAST>>& GetASTs() const { return m_asts; }

	void SetWorkspace(std::map<std::string, std::shared_ptr<Symbol>> *ws) { m_workspace = ws; }
	std::map<std::string, std::shared_ptr<Symbol>> * GetWorkspace() { return m_workspace; }

	void EmitWorkspaceUpdated(const std::string& ident="") { m_WorkspaceUpdated(ident); }
	boost::signals2::signal<void(const std::string&)>& GetWorkspaceUpdatedSignal() { return m_WorkspaceUpdated; }
};

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// Symbols
// ----------------------------------------------------------------------------

enum class SymbolType
{
	REAL,	// e.g. 12.3
	STRING,	// e.g. "abc"
	LIST,	// e.g. 1, 2, 3
	ARRAY,	// e.g. [1, 2, 3]
	MAP,	// e.g. {"key" : 1.23}
	DATASET
};


class Symbol
{
public:
	virtual ~Symbol() {}

	virtual SymbolType GetType() const = 0;
	virtual std::shared_ptr<Symbol> copy() const = 0;
	virtual void print(std::ostream& ostr) const = 0;

	virtual std::string serialise() const = 0;
	static std::shared_ptr<Symbol> unserialise(const std::string &str);

	static std::shared_ptr<Symbol> uminus(const Symbol &sym2);
	static std::shared_ptr<Symbol> add(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> sub(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> mul(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> div(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> mod(const Symbol &sym1, const Symbol &sym2);
	static std::shared_ptr<Symbol> pow(const Symbol &sym1, const Symbol &sym2);

	static std::shared_ptr<Symbol> add(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2);
	static std::shared_ptr<Symbol> sub(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2);
	static std::shared_ptr<Symbol> mul(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2);
	static std::shared_ptr<Symbol> div(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2);

	static const std::string& get_type_name(const Symbol &sym);
};


class SymbolReal : public Symbol
{
private:
	t_real_cli m_val = 0;

public:
	SymbolReal() = default;
	SymbolReal(t_real_cli val) : m_val(val) {}
	virtual ~SymbolReal() {}

	virtual SymbolType GetType() const override { return SymbolType::REAL; }
	t_real_cli GetValue() const { return m_val; }
	void SetValue(t_real_cli val) { m_val = val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolReal>(m_val); }
	virtual void print(std::ostream& ostr) const override { ostr << GetValue(); }
	virtual std::string serialise() const override;
};


class SymbolString : public Symbol
{
private:
	std::string m_val;

public:
	SymbolString() = default;
	SymbolString(const std::string& val) : m_val(val) {}
	SymbolString(std::string&& val) : m_val(val) {}
	virtual ~SymbolString() {}

	virtual SymbolType GetType() const override { return SymbolType::STRING; }
	const std::string& GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolString>(m_val); }
	virtual void print(std::ostream& ostr) const override { ostr << GetValue(); }
	virtual std::string serialise() const override;
};


class SymbolList : public Symbol
{
private:
	std::vector<std::shared_ptr<Symbol>> m_val;
	bool m_islist = true;	// list or array

public:
	SymbolList() = default;
	SymbolList(const std::vector<std::shared_ptr<Symbol>>& val, bool islist=1) : m_val(val), m_islist(islist) {}
	SymbolList(std::vector<std::shared_ptr<Symbol>>&& val, bool islist=1) : m_val(val), m_islist(islist) {}
	SymbolList(const std::initializer_list<std::shared_ptr<Symbol>>& lst, bool islist=1) : m_islist(islist)
	{
		for(auto iter = lst.begin(); iter !=lst.end(); ++iter)
			m_val.push_back(*iter);
	}
	virtual ~SymbolList() {}

	virtual SymbolType GetType() const override { return m_islist ? SymbolType::LIST : SymbolType::ARRAY; }
	const std::vector<std::shared_ptr<Symbol>>& GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolList>(m_val, m_islist); }

	virtual void print(std::ostream& ostr) const override
	{
		if(!m_islist) ostr << "[ ";
		for(std::size_t i=0; i<GetValue().size(); ++i)
		{
			GetValue()[i]->print(ostr);
			if(i < GetValue().size()-1)
				ostr << ", ";
		}
		if(!m_islist) ostr << " ]";
	}

	virtual std::string serialise() const override;
};


class SymbolDataset : public Symbol
{
private:
	Dataset m_val;

public:
	SymbolDataset() = default;
	SymbolDataset(const Dataset& val) : m_val(val) {}
	SymbolDataset(Dataset&& val) : m_val(val) {}
	virtual ~SymbolDataset() {}

	virtual SymbolType GetType() const override { return SymbolType::DATASET; }
	const Dataset& GetValue() const { return m_val; }

	virtual std::shared_ptr<Symbol> copy() const override { return std::make_shared<SymbolDataset>(m_val); }
	virtual void print(std::ostream& ostr) const override { ostr << "&lt;Dataset&gt;"; }
	virtual std::string serialise() const override;
};



/**
 * write symbol to ostream
 */
static inline std::ostream& operator<< (std::ostream& ostr, const Symbol& sym)
{
	sym.print(ostr);
	return ostr;
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// AST
// ----------------------------------------------------------------------------

enum class CliASTType
{
	REAL,
	STRING,
	IDENT,
	ASSIGN,
	PLUS,
	MINUS,
	MULT,
	DIV,
	MOD,
	POW,
	CALL,
	EXPRLIST,
	ARRAY,
	ARRAYACCESS,
};


class CliAST
{
protected:
	std::shared_ptr<CliAST> m_left;
	std::shared_ptr<CliAST> m_right;

public:
	CliAST(std::shared_ptr<CliAST> left=nullptr, std::shared_ptr<CliAST> right=nullptr) : m_left(left), m_right(right) {}
	virtual ~CliAST() {}

	void SetLeft(std::shared_ptr<CliAST> left) { m_left = left; }
	void SetRight(std::shared_ptr<CliAST> right) { m_right = right; }

	virtual void Print(std::ostringstream &ostr, int indent = 0) const;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const = 0;
	virtual CliASTType GetType() const = 0;
};


class CliASTReal : public CliAST
{
protected:
	t_real_cli m_val = t_real_cli(0);

public:
	CliASTReal(t_real_cli val) : m_val(val) { }
	virtual ~CliASTReal() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::REAL; }
	t_real_cli GetValue() const { return m_val; }
};


class CliASTString : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTString(const std::string& val) : m_val(val) { }
	virtual ~CliASTString() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::STRING; }
	const std::string& GetValue() const { return m_val; }
};


class CliASTIdent : public CliAST
{
protected:
	std::string m_val;

public:
	CliASTIdent(const std::string& val) : m_val(val) { }
	virtual ~CliASTIdent() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::IDENT; }
	const std::string& GetValue() const { return m_val; }
};


class CliASTAssign : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTAssign() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::ASSIGN; }
};


class CliASTPlus : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTPlus() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::PLUS; }
};


class CliASTMinus : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTMinus() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::MINUS; }
};


class CliASTMult : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTMult() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::MULT; }
};


class CliASTDiv : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTDiv() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::DIV; }
};


class CliASTMod : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTMod() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::MOD; }
};


class CliASTPow : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTPow() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::POW; }
};


class CliASTCall : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTCall() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::CALL; }
};


class CliASTExprList : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTExprList() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::EXPRLIST; }
};


class CliASTArray : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTArray() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::ARRAY; }
};


class CliASTArrayAccess : public CliAST
{
public:
	using CliAST::CliAST;
	virtual ~CliASTArrayAccess() {}

	virtual void Print(std::ostringstream &ostr, int indent = 0) const override;
	virtual std::shared_ptr<Symbol> Eval(CliParserContext& ctx) const override;

	virtual CliASTType GetType() const override { return CliASTType::ARRAYACCESS; }
};


// ----------------------------------------------------------------------------



#undef YY_DECL
#define YY_DECL yy::CliParser::symbol_type CliLexer::yylex(CliParserContext &ctx)
extern yy::CliParser::symbol_type yylex(CliParserContext &ctx);

#define yyterminate() return yy::CliParser::token::yytokentype(YY_NULL);


#endif
