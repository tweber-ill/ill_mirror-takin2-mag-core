/**
 * tlibs2
 * logger/debug library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date Sep-2014 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_LOGGER_H__
#define __TLIBS2_LOGGER_H__

#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <utility>
#include <exception>
#include <boost/type_index.hpp>

#include "algos.h"


namespace tl2 {


template<typename T>
std::string get_typename(bool bFull=1)
{
	boost::typeindex::type_index idx;

	if(bFull)
		idx = boost::typeindex::type_id_with_cvr<T>();
	else
		idx = boost::typeindex::type_id<T>();

	return idx.pretty_name();
}


enum class LogColor
{
	NONE,
	RED, BLUE, GREEN,
	YELLOW, PURPLE, CYAN,
	WHITE, BLACK
};

class Log
{
private:
	int m_iDepth = 0;

protected:
	static std::recursive_mutex s_mtx;

	// pair of ostream and colour flag
	using t_pairOstr = std::pair<std::ostream*, bool>;
	std::vector<t_pairOstr> m_vecOstrs;
	using t_mapthreadOstrs = std::unordered_map<std::thread::id, std::vector<t_pairOstr>>;
	t_mapthreadOstrs m_mapOstrsTh;

	std::string m_strInfo;
	LogColor m_col = LogColor::NONE;

	bool m_bEnabled = 1;
	bool m_bShowDate = 1;

	bool m_bShowThread = 0;
	unsigned int m_iNumThreads = 0;

	static bool s_bTermCmds;

protected:
	static std::string get_timestamp();
	static std::string get_thread_id();
	static std::string get_color(LogColor col, bool bBold=0);

	std::vector<t_pairOstr>& GetThreadOstrs();

	void begin_log();
	void end_log();

	void inc_depth();
	void dec_depth();

public:
	Log();
	Log(const std::string& strInfo, LogColor col, std::ostream* = nullptr);
	~Log();

	void AddOstr(std::ostream* pOstr, bool bCol=1, bool bThreadLocal=0);
	void RemoveOstr(std::ostream* pOstr);

	template<typename ...t_args>
	void operator()(t_args&&... args)
	{
		if(!m_bEnabled) return;
		begin_log();

		std::vector<t_pairOstr>& vecOstrsTh = GetThreadOstrs();
		std::vector<t_pairOstr> vecOstrs = arrayunion({m_vecOstrs, vecOstrsTh});

		for(t_pairOstr& pair : vecOstrs)
		{
			if(pair.first)
				((*pair.first) << ... << std::forward<t_args>(args));
		}
		end_log();
	}

	void SetEnabled(bool bEnab) { m_bEnabled = bEnab; }
	void SetShowDate(bool bDate) { m_bShowDate = bDate; }
	void SetShowThread(bool bThread) { m_bShowThread = bThread; }

	static void SetUseTermCmds(bool bCmds) { s_bTermCmds = bCmds; }
};


extern Log log_info, log_warn, log_err, log_crit, log_debug;


class Err : public std::exception
{
	protected:
		std::string m_strErr;

	public:
		Err(const std::string& strErr, bool bErrStr=0) noexcept
			: m_strErr((bErrStr? "Exception: " : "") + strErr)
		{}

		Err(const char* pcErr) noexcept : Err(std::string(pcErr))
		{}

		virtual ~Err() noexcept
		{}

		virtual const char* what() const noexcept override
		{
			return m_strErr.c_str();
		}
};

}
#endif
