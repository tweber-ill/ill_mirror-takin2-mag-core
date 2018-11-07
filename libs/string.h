/**
 * tlibs2
 * string library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date 2013 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_STRINGS__
#define __TLIBS2_STRINGS__

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <locale>
#include <limits>
#include <map>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <cctype>
#include <cwctype>
#include <unordered_map>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/math/special_functions/erf.hpp>

#include "log.h"
#include "units.h"


namespace algo = boost::algorithm;


namespace tl2 {

template<class t_str/*=std::string*/, class t_val/*=double*/>
std::pair<bool, t_val> eval_expr(const t_str& str) noexcept;


// -----------------------------------------------------------------------------

template<class t_str=std::string> const t_str& get_dir_seps();
template<class t_str=std::string> const t_str& get_trim_chars();

template<> inline const std::string& get_dir_seps()
{
	static const std::string strSeps("\\/");
	return strSeps;
}
template<> inline const std::wstring& get_dir_seps()
{
	static const std::wstring strSeps(L"\\/");
	return strSeps;
}

template<> inline const std::string& get_trim_chars()
{
	static const std::string strC(" \t\r");
	return strC;
}
template<> inline const std::wstring& get_trim_chars()
{
	static const std::wstring strC(L" \t\r");
	return strC;
}


// -----------------------------------------------------------------------------


static inline std::wstring str_to_wstr(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

static inline std::string wstr_to_str(const std::wstring& str)
{
	return std::string(str.begin(), str.end());
}


// overloaded in case the string is already of correct type
static inline const std::wstring& str_to_wstr(const std::wstring& str) { return str; }
static inline const std::string& wstr_to_str(const std::string& str) { return str; }


// -----------------------------------------------------------------------------


template<class t_str=std::string>
t_str str_to_upper(const t_str& str)
{
	t_str strOut;
	std::locale loc;

	strOut = boost::to_upper_copy(str, loc);
	return strOut;
}

template<class t_str=std::string>
t_str str_to_lower(const t_str& str)
{
	t_str strLower;
	std::locale loc;

	strLower = boost::to_lower_copy(str, loc);
	return strLower;
}


// -----------------------------------------------------------------------------


template<class t_str=std::string>
t_str get_file_noext(const t_str& str, bool bToLower=0)
{
	std::size_t iPos = str.find_last_of('.');

	if(iPos == t_str::npos)
		return str;

	t_str strRet = str.substr(0, iPos);
	if(bToLower)
		strRet = str_to_lower(strRet);

	return strRet;
}

template<class t_str=std::string>
t_str get_fileext(const t_str& str, bool bToLower=0)
{
	std::size_t iPos = str.find_last_of('.');

	if(iPos == t_str::npos)
		return t_str();

	t_str strRet = str.substr(iPos+1);
	if(bToLower)
		strRet = str_to_lower(strRet);

	return strRet;
}

/**
 *  e.g. returns "tof" for "123.tof.bz2"
 */
template<class t_str=std::string>
t_str get_fileext2(const t_str& str, bool bToLower=0)
{
	std::size_t iPos = str.find_last_of('.');
	if(iPos == t_str::npos || iPos == 0)
		return t_str();

	t_str strFile = str.substr(0, iPos);
	return get_fileext(strFile, bToLower);
}

/**
 * e.g. returns "tof" for "123.tof.bz2" and for "123.tof"
 */
template<class t_str=std::string>
t_str get_fileext_nocomp(const t_str& str, bool bToLower=0)
{
	std::size_t iCnt = std::count(str.begin(), str.end(), '.');
	if(iCnt==0)
		return t_str();
	else if(iCnt==1)
		return get_fileext(str, bToLower);
	else
		return get_fileext2(str, bToLower);
}

template<class t_str=std::string>
t_str get_dir(const t_str& str, bool bToLower=0)
{
	std::size_t iPos = str.find_last_of(get_dir_seps<t_str>());

	if(iPos == t_str::npos)
		return t_str();

	t_str strRet = str.substr(0, iPos);
	if(bToLower)
		strRet = str_to_lower(strRet);

	return strRet;
}

template<class t_str=std::string>
t_str get_file_nodir(const t_str& str, bool bToLower=0)
{
	std::size_t iPos = str.find_last_of(get_dir_seps<t_str>());

	if(iPos == t_str::npos)
		return t_str();

	t_str strRet = str.substr(iPos+1);
	if(bToLower)
		strRet = str_to_lower(strRet);

	return strRet;
}


// -----------------------------------------------------------------------------



template<class t_str=std::string>
bool str_is_equal(const t_str& str0, const t_str& str1, bool bCase=0)
{
	if(bCase)
		return algo::equals(str0, str1, algo::is_equal());
	else
		return algo::equals(str0, str1, algo::is_iequal(std::locale()));
}

template<class t_str=std::string>
bool str_is_equal_to_either(const t_str& str0,
	const std::initializer_list<t_str>& lststr1, bool bCase=0)
{
	for(const t_str& str1 : lststr1)
		if(str_is_equal<t_str>(str0, str1, bCase))
			return true;
	return false;
}


template<class t_str=std::string>
bool str_contains(const t_str& str, const t_str& strSub, bool bCase=0)
{
	if(bCase)
		return algo::contains(str, strSub, algo::is_equal());
	else
		return algo::contains(str, strSub, algo::is_iequal(std::locale()));
}


// -----------------------------------------------------------------------------


template<class t_str=std::string>
void trim(t_str& str)
{
	using t_char = typename t_str::value_type;

	boost::trim_if(str, [](t_char c) -> bool
	{
		return get_trim_chars<t_str>().find(c) != t_str::npos;
	});
}

template<class t_str=std::string>
t_str trimmed(const t_str& str)
{
	t_str strret = str;
	trim(strret);
	return strret;
}


// -----------------------------------------------------------------------------



/**
 * removes all occurrences of a char in a string
 */
template<class t_str=std::string>
t_str remove_char(const t_str& str, typename t_str::value_type ch)
{
	t_str strRet;

	for(typename t_str::value_type c : str)
		if(c != ch)
			strRet.push_back(c);

	return strRet;
}

/**
 * removes all occurrences of specified chars in a string
 */
template<class t_str=std::string>
t_str remove_chars(const t_str& str, const t_str& chs)
{
	t_str strRet;

	for(typename t_str::value_type c : str)
	{
		bool bRemove = 0;
		for(typename t_str::value_type ch : chs)
		{
			if(c == ch)
			{
				bRemove = 1;
				break;
			}
		}

		if(!bRemove)
			strRet.push_back(c);
	}

	return strRet;
}


/**
 * Removes substring between strStart and strEnd
 * @return Number of removed substrings
 */
template<class t_str = std::string>
std::size_t string_rm(t_str& str, const t_str& strStart, const t_str& strEnd)
{
	std::size_t iNumFound = 0;

	while(1)
	{
		std::size_t iStart = str.find(strStart);
		std::size_t iEnd = str.find(strEnd);

		if(iStart == t_str::npos || iEnd == t_str::npos)
			break;
		if(iStart >= iEnd)
			break;

		str.erase(iStart, iEnd-iStart+strEnd.length());
		++iNumFound;
	}

	return iNumFound;
}


// -----------------------------------------------------------------------------


template<class t_str=std::string>
t_str insert_before(const t_str& str, const t_str& strChar, const t_str& strInsert)
{
	std::size_t pos = str.find(strChar);
	if(pos==t_str::npos)
		return str;

	t_str strRet = str;
	strRet.insert(pos, strInsert);

	return strRet;
}


template<class t_str=std::string>
bool begins_with(const t_str& str, const t_str& strBeg, bool bCase=1)
{
	if(bCase)
		return algo::starts_with(str, strBeg, algo::is_equal());
	else
		return algo::starts_with(str, strBeg, algo::is_iequal(std::locale()));
}


template<class t_str=std::string>
bool ends_with(const t_str& str, const t_str& strEnd, bool bCase=1)
{
	if(bCase)
		return algo::ends_with(str, strEnd, algo::is_equal());
	else
		return algo::ends_with(str, strEnd, algo::is_iequal(std::locale()));
}


// -----------------------------------------------------------------------------


template<class t_str=std::string>
std::pair<t_str, t_str>
split_first(const t_str& str, const t_str& strSep, bool bTrim=0, bool bSeq=0)
{
	t_str str1, str2;

	std::size_t iLenTok = bSeq ? strSep.length() : 1;
	std::size_t ipos = bSeq ? str.find(strSep) : str.find_first_of(strSep);

	if(ipos != t_str::npos)
	{
		str1 = str.substr(0, ipos);
		if(ipos+iLenTok < str.length())
			str2 = str.substr(ipos+iLenTok, t_str::npos);
	}
	//else
	//	str1 = str;

	if(bTrim)
	{
		trim(str1);
		trim(str2);
	}

	return std::pair<t_str, t_str>(str1, str2);
}


/**
 * get string between strSep1 and strSep2
 */
template<class t_str=std::string>
t_str str_between(const t_str& str, const t_str& strSep1, const t_str& strSep2,
	bool bTrim=0, bool bSeq=0)
{
	t_str str1, str2;
	std::tie(str1, str2) = split_first<t_str>(str, strSep1, bTrim, bSeq);
	if(str2 == "") return t_str("");

	std::tie(str1, str2) = split_first<t_str>(str2, strSep2, bTrim, bSeq);
	return str1;
}


// ----------------------------------------------------------------------------


template<typename T, class t_str=std::string, bool bTIsStr=0>
struct _str_to_var_impl;

template<typename T, class t_str>
struct _str_to_var_impl<T, t_str, 1>
{
	inline const T& operator()(const t_str& str) const
	{
		return str;
	}
};

template<typename T, class t_str>
struct _str_to_var_impl<T, t_str, 0>
{
	inline T operator()(const t_str& str) const
	{
		if(!trimmed(str).length())
			return T();

		typedef typename t_str::value_type t_char;
		std::basic_istringstream<t_char> istr(str);

		T t;
		istr >> t;
		return t;
	}
};



/**
 * Tokenises string on any of the chars in strDelim
 */
template<class T, class t_str=std::string, class t_cont=std::vector<T>>
void get_tokens(const t_str& str, const t_str& strDelim, t_cont& vecRet)
{
	using t_char = typename t_str::value_type;
	using t_tokeniser = boost::tokenizer<boost::char_separator<t_char>,
		typename t_str::const_iterator, t_str>;
	using t_tokiter = typename t_tokeniser::iterator;

	boost::char_separator<t_char> delim(strDelim.c_str());
	t_tokeniser tok(str, delim);

	for(t_tokiter iter=tok.begin(); iter!=tok.end(); ++iter)
	{
		vecRet.push_back(
			_str_to_var_impl<T, t_str,
			std::is_convertible<T, t_str>::value>()(*iter));
	}
}

/**
 * Tokenises string on strDelim
 */
template<class T, class t_str=std::string, template<class...> class t_cont=std::vector>
void get_tokens_seq(const t_str& str, const t_str& strDelim, t_cont<T>& vecRet, bool bCase=1)
{
	using t_char = typename t_str::value_type;

	std::locale loc;
	t_cont<t_str> vecStr;
	algo::iter_split(vecStr, str, algo::first_finder(strDelim,
		[bCase, &loc](t_char c1, t_char c2) -> bool
		{
			if(!bCase)
			{
				c1 = std::tolower(c1, loc);
				c2 = std::tolower(c2, loc);
			}

			return c1 == c2;
		}));

	for(const t_str& strTok : vecStr)
	{
		vecRet.push_back(
			_str_to_var_impl<T, t_str,
			std::is_convertible<T, t_str>::value>()(strTok));
	}
}


template<class T, class t_str=std::string, class t_cont=std::vector<T>>
bool parse_tokens(const t_str& str, const t_str& strDelim, t_cont& vecRet)
{
	std::vector<t_str> vecStrs;
	get_tokens<t_str, t_str, std::vector<t_str>>(str, strDelim, vecStrs);

	bool bOk = 1;
	for(const t_str& str : vecStrs)
	{
		std::pair<bool, T> pairResult = eval_expr<t_str, T>(str);
		vecRet.push_back(pairResult.second);
		if(!pairResult.first) bOk = 0;
	}

	return bOk;
}


template<typename T, class t_str=std::string>
T str_to_var_parse(const t_str& str)
{
	std::pair<bool, T> pairResult = eval_expr<t_str, T>(str);
	if(!pairResult.first)
		return T(0);
	return pairResult.second;
}


template<typename T, class t_str=std::string>
T str_to_var(const t_str& str)
{
	return _str_to_var_impl<T, t_str, std::is_convertible<T, t_str>::value>()(str);
}


// ----------------------------------------------------------------------------


template<class T, class t_ch,
	bool is_number_type=std::is_fundamental<T>::value>
struct _var_to_str_print_impl {};

template<class T, class t_ch> struct _var_to_str_print_impl<T, t_ch, false>
{
	void operator()(std::basic_ostream<t_ch>& ostr, const T& t) { ostr << t; }
};

template<class T, class t_ch> struct _var_to_str_print_impl<T, t_ch, true>
{
	void operator()(std::basic_ostream<t_ch>& ostr, const T& t)
	{
		// prevents printing "-0"
		T t0 = t;
		if(t0==T(-0)) t0 = T(0);

		ostr << t0;
	}
};

template<typename T, class t_str=std::string>
struct _var_to_str_impl
{
	t_str operator()(const T& t,
		std::streamsize iPrec = std::numeric_limits<T>::max_digits10,
		int iGroup=-1)
	{
		//if(std::is_convertible<T, t_str>::value)
		//	return *reinterpret_cast<const t_str*>(&t);

		typedef typename t_str::value_type t_char;

		std::basic_ostringstream<t_char> ostr;
		ostr.precision(iPrec);


		class Sep : public std::numpunct<t_char>
		{
		public:
			Sep() : std::numpunct<t_char>(1) {}
			~Sep() { /*std::cout << "~Sep();" << std::endl;*/ }
		protected:
			virtual t_char do_thousands_sep() const override { return ' ';}
			virtual std::string do_grouping() const override { return "\3"; }
		};
		Sep *pSep = nullptr;


		if(iGroup > 0)
		{
			pSep = new Sep();
			ostr.imbue(std::locale(ostr.getloc(), pSep));
		}

		_var_to_str_print_impl<T, t_char> pr;
		pr(ostr, t);
		t_str str = ostr.str();

		if(pSep)
		{
			ostr.imbue(std::locale());
			delete pSep;
		}
		return str;
	}
};

template<class t_str>
struct _var_to_str_impl<t_str, t_str>
{
	const t_str& operator()(const t_str& tstr, std::streamsize iPrec=10, int iGroup=-1)
	{
		return tstr;
	}

	t_str operator()(const typename t_str::value_type* pc, std::streamsize iPrec=10, int iGroup=-1)
	{
		return t_str(pc);
	}
};

template<typename T, class t_str=std::string>
t_str var_to_str(const T& t,
	std::streamsize iPrec = std::numeric_limits<T>::max_digits10-1,
	int iGroup = -1)
{
	_var_to_str_impl<T, t_str> _impl;
	return _impl(t, iPrec, iGroup);
}


/**
 * converts a container (e.g. a vector) to a string
 */
template<class t_cont, class t_str=std::string>
t_str cont_to_str(const t_cont& cont, const char* pcDelim=",",
	std::streamsize iPrec = std::numeric_limits<typename t_cont::value_type>::max_digits10-1)
{
	using t_elem = typename t_cont::value_type;

	t_str str;

	for(typename t_cont::const_iterator iter=cont.begin(); iter!=cont.end(); ++iter)
	{
		const t_elem& elem = *iter;

		str += var_to_str<t_elem, t_str>(elem, iPrec);
		if(iter+1 != cont.end())
			str += pcDelim;
	}
	return str;
}

// ----------------------------------------------------------------------------


template<typename t_char=char>
bool skip_after_line(std::basic_istream<t_char>& istr,
	const std::basic_string<t_char>& strLineBegin,
	bool bTrim=true, bool bCase=0)
{
	while(!istr.eof())
	{
		std::basic_string<t_char> strLine;
		std::getline(istr, strLine);
		if(bTrim)
			trim(strLine);

		if(strLine.size() < strLineBegin.size())
			continue;

		std::basic_string<t_char> strSub = strLine.substr(0, strLineBegin.size());

		if(str_is_equal<std::basic_string<t_char>>(strSub, strLineBegin, bCase))
			return true;
	}
	return false;
}

template<typename t_char=char>
void skip_after_char(std::basic_istream<t_char>& istr, t_char ch, bool bCase=0)
{
	std::locale loc;
	if(!bCase) ch = std::tolower(ch, loc);

	while(!istr.eof())
	{
		t_char c;
		istr.get(c);

		if(!bCase) c = std::tolower(c, loc);

		if(c == ch)
			break;
	}
}


// ----------------------------------------------------------------------------


/**
 * functions working on chars
 */
template<class t_ch, typename=void> struct char_funcs {};

/**
 * specialisation for char
 */
template<class t_ch>
struct char_funcs<t_ch, typename std::enable_if<std::is_same<t_ch, char>::value>::type>
{
	static bool is_digit(t_ch c) { return std::isdigit(c); }
};

/**
 * specialisation for wchar_t
 */
template<class t_ch>
struct char_funcs<t_ch, typename std::enable_if<std::is_same<t_ch, wchar_t>::value>::type>
{
	static bool is_digit(t_ch c) { return std::iswdigit(c); }
};


// ----------------------------------------------------------------------------


/**
 * tests if a string consists entirely of numbers
 */
template<class t_str = std::string>
bool str_is_digits(const t_str& str)
{
	using t_ch = typename t_str::value_type;
	using t_fkt = char_funcs<t_ch>;

	bool bAllNums = std::all_of(str.begin(), str.end(),
		[](t_ch c) -> bool
		{
			return t_fkt::is_digit(c);
		});
	return bAllNums;
}



// ----------------------------------------------------------------------------



template<class t_str=std::string, class t_cont=std::vector<double>>
t_cont get_py_array(const t_str& str)
{
	typedef typename t_cont::value_type t_elems;
	t_cont vecArr;

	std::size_t iStart = str.find('[');
	std::size_t iEnd = str.find(']');

	// search for list instead
	if(iStart==t_str::npos || iEnd==t_str::npos)
	{
		iStart = str.find('(');
		iEnd = str.find(')');
	}

	// invalid array
	if(iStart==t_str::npos || iEnd==t_str::npos || iEnd<iStart)
		return vecArr;

	t_str strArr = str.substr(iStart+1, iEnd-iStart-1);
	get_tokens<t_elems, t_str>(strArr, ",", vecArr);

	return vecArr;
}


template<class t_str=std::string>
t_str get_py_string(const t_str& str)
{
	std::size_t iStart = str.find_first_of("\'\"");
	std::size_t iEnd = str.find_last_of("\'\"");

	// invalid string
	if(iStart==t_str::npos || iEnd==t_str::npos || iEnd<iStart)
		return "";

	return str.substr(iStart+1, iEnd-iStart-1);
}



// ----------------------------------------------------------------------------



template<class t_str=std::string, class t_val=double> std::pair<bool, t_val>
	eval_expr(const t_str& str) noexcept;


// real functions with one parameter
template<class t_str, class t_val,
	typename std::enable_if<std::is_floating_point<t_val>::value>::type* =nullptr>
t_val call_func1(const t_str& strName, t_val t)
{
	//std::cout << "calling " << strName << " with arg " << t << std::endl;
	static const std::unordered_map</*t_str*/std::string, t_val(*)(t_val)> s_funcs =
	{
		{ "sin", std::sin }, { "cos", std::cos }, { "tan", std::tan },
		{ "asin", std::asin }, { "acos", std::acos }, { "atan", std::atan },
		{ "sinh", std::sinh }, { "cosh", std::cosh }, { "tanh", std::tanh },
		{ "asinh", std::asinh }, { "acosh", std::acosh }, { "atanh", std::atanh },

		{ "sqrt", std::sqrt }, { "cbrt", std::cbrt },
		{ "exp", std::exp },
		{ "log", std::log }, { "log2", std::log2 }, { "log10", std::log10 },

		{ "erf", std::erf }, { "erfc", std::erfc }, { "erf_inv", boost::math::erf_inv },

		{ "round", std::round }, { "ceil", std::ceil }, { "floor", std::floor },
		{ "abs", std::abs },
	};

	return s_funcs.at(wstr_to_str(strName))(t);
}

// real functions with two parameters
template<class t_str, class t_val,
	typename std::enable_if<std::is_floating_point<t_val>::value>::type* =nullptr>
t_val call_func2(const t_str& strName, t_val t1, t_val t2)
{
	static const std::unordered_map</*t_str*/std::string, t_val(*)(t_val, t_val)> s_funcs =
	{
		{ "pow", std::pow }, { "atan2", std::atan2 },
		{ "mod", std::fmod },
	};

	return s_funcs.at(wstr_to_str(strName))(t1, t2);
}

// real constants
template<class t_str, class t_val,
	typename std::enable_if<std::is_floating_point<t_val>::value>::type* =nullptr>
t_val get_const(const t_str& strName)
{
	static const std::unordered_map</*t_str*/std::string, t_val> s_consts =
	{
		{ "pi", __pi<t_val> },
		{ "hbar",  t_val(hbar<t_val>/meV<t_val>/sec<t_val>) },	// hbar in [meV s]
		{ "kB",  t_val(kB<t_val>/meV<t_val>*kelvin<t_val>) },	// kB in [meV / K]
	};

	return s_consts.at(wstr_to_str(strName));
}


// alternative: int functions with one parameter
template<class t_str, class t_val,
	typename std::enable_if<std::is_integral<t_val>::value>::type* =nullptr>
t_val call_func1(const t_str& strName, t_val t)
{
	static const std::unordered_map</*t_str*/std::string, t_val(*)(t_val)> s_funcs =
	{
		{ "abs", std::abs },
	};

	return s_funcs.at(wstr_to_str(strName))(t);
}

// alternative: int functions with two parameters
template<class t_str, class t_val,
	typename std::enable_if<std::is_integral<t_val>::value>::type* =nullptr>
t_val call_func2(const t_str& strName, t_val t1, t_val t2)
{
	static const std::unordered_map</*t_str*/std::string, std::function<t_val(t_val, t_val)>> s_funcs =
	{
		{ "pow", [t1, t2](t_val t1, t_val t2) -> t_val { return t_val(std::pow(t1, t2)); } },
		{ "mod", [t1, t2](t_val t1, t_val t2) -> t_val { return t1%t2; } },
	};

	return s_funcs.at(wstr_to_str(strName))(t1, t2);
}

// alternative: int constants
template<class t_str, class t_val,
	typename std::enable_if<std::is_integral<t_val>::value>::type* =nullptr>
t_val get_const(const t_str& strName)
{
	/*static const std::unordered_map<std::string, t_val> s_consts =
	{
		{ "pi", 3 },
	};

	return s_consts.at(wstr_to_str(strName));*/

	throw std::out_of_range("Undefined constant.");
}


namespace sp = boost::spirit;
namespace qi = boost::spirit::qi;
namespace asc = boost::spirit::ascii;
namespace ph = boost::phoenix;

template<class t_str, class t_val, class t_skip=asc::space_type>
class ExprGrammar : public qi::grammar<
	typename t_str::const_iterator, t_val(), t_skip>
{
	protected:
		using t_ch = typename t_str::value_type;
		using t_iter = typename t_str::const_iterator;
		using t_valparser = typename std::conditional<
			std::is_floating_point<t_val>::value,
			qi::real_parser<t_val>, qi::int_parser<t_val>>::type;

		qi::rule<t_iter, t_val(), t_skip> m_expr, m_term;
		qi::rule<t_iter, t_val(), t_skip> m_val, m_baseval, m_const;
		qi::rule<t_iter, t_val(), t_skip> m_func1, m_func2;
		qi::rule<t_iter, t_val(), t_skip> m_pm, m_pm_opt, m_p, m_m;
		qi::rule<t_iter, t_str(), t_skip> m_ident;

		void SetErrorHandling()
		{
			m_expr.name("expr");
			m_term.name("term");
			m_val.name("val");
			m_baseval.name("baseval");
			m_const.name("const");
			m_func1.name("func_1arg");
			m_func2.name("func_2args");
			m_pm.name("plusminus");
			m_pm_opt.name("plusminus_opt");
			m_p.name("plus");
			m_m.name("minus");
			m_ident.name("ident");

			qi::on_error<qi::fail>(m_expr,
				ph::bind([](t_iter beg, t_iter err, t_iter end, const sp::info& infoErr)
				{
					std::string strBeg(beg, err);
					std::string strEnd(err, end);
					std::string strRem;
					if(strEnd.length())
						strRem = "remaining \"" + strEnd + "\", ";

					log_err("Parsed \"", strBeg, "\", ", strRem,
						"expected token ", infoErr, ".");
				}, qi::labels::_1, qi::labels::_3, qi::labels::_2, qi::labels::_4));
		}

	public:
		ExprGrammar() : ExprGrammar::base_type(m_expr, "expr")
		{
			// + or -
			m_expr = ((m_pm_opt > m_term) [ qi::_val = qi::_1*qi::_2  ]
				> *((m_p|m_m) > m_term) [ qi::_val += qi::_1*qi::_2 ]);

			m_pm_opt = (m_p | m_m) [ qi::_val = qi::_1 ]
				| qi::eps [ qi::_val = t_val(1) ];
			m_p = qi::char_(t_ch('+')) [ qi::_val = t_val(1) ];
			m_m = qi::char_(t_ch('-')) [ qi::_val = t_val(-1) ];

			// * or /
			m_term = (m_val [ qi::_val = qi::_1 ]
				> *((t_ch('*') > m_val) [ qi::_val *= qi::_1 ]
					| (t_ch('/') > m_val) [ qi::_val /= qi::_1 ]))
				| m_val [ qi::_val = qi::_1 ];

			// pow
			m_val = m_baseval [ qi::_val = qi::_1 ]
				> *((t_ch('^') > m_baseval)
				[ qi::_val = ph::bind([](t_val val1, t_val val2) -> t_val
				{ return std::pow(val1, val2); }, qi::_val, qi::_1)]);

			m_baseval = t_valparser() | m_func2 | m_func1 | m_const
				| (t_ch('(') > m_expr > t_ch(')'));

			// lazy evaluation of constants via phoenix bind
			m_const = m_ident [ qi::_val = ph::bind([](const t_str& strName) -> t_val
				{ return get_const<t_str, t_val>(strName); }, qi::_1) ];

			// lazy evaluation of functions via phoenix bind
			m_func2 = (m_ident >> t_ch('(') >> m_expr >> t_ch(',') >> m_expr >> t_ch(')'))
				[ qi::_val = ph::bind([](const t_str& strName, t_val val1, t_val val2) -> t_val
				{ return call_func2<t_str, t_val>(strName, val1, val2); },
				qi::_1, qi::_2, qi::_3) ];
			m_func1 = ((m_ident >> t_ch('(') >> m_expr >> t_ch(')')))
				[ qi::_val = ph::bind([](const t_str& strName, t_val val) -> t_val
				{ return call_func1<t_str, t_val>(strName, val); },
				qi::_1, qi::_2) ];

			m_ident = qi::lexeme[qi::char_("A-Za-z_") > *qi::char_("0-9A-Za-z_")];

			SetErrorHandling();
		}

		~ExprGrammar() {}
};

template<class t_str/*=std::string*/, class t_val/*=double*/>
std::pair<bool, t_val> eval_expr(const t_str& str) noexcept
{
	if(trimmed(str).length() == 0)
		return std::make_pair(true, t_val(0));

	try
	{
		using t_iter = typename t_str::const_iterator;
		t_iter beg = str.begin(), end = str.end();
		t_val valRes(0);

		ExprGrammar<t_str, t_val> gram;
		bool bOk = qi::phrase_parse(beg, end, gram, asc::space, valRes);
		if(beg != end)
		{
			//std::string strErr(beg, end);
			//log_err("Not all tokens were parsed. Remaining: \"", strErr, "\".");
			bOk = 0;
		}
		return std::make_pair(bOk, valRes);
	}
	catch(const std::exception& ex)
	{
		log_err("Parsing failed with error: ", ex.what(), ".");
		return std::make_pair(false, t_val(0));
	}
}

}
#endif
