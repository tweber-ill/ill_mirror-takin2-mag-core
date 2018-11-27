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

#include <cstring>
#include <cmath>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <utility>
#include <exception>
#include "algos.h"

#include <boost/type_index.hpp>


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


// ----------------------------------------------------------------------------


template<class T = double>
class Stopwatch
{
	public:
		typedef std::chrono::system_clock::time_point t_tp_sys;
		typedef std::chrono::steady_clock::time_point t_tp_st;
		typedef std::chrono::duration<T> t_dur;
		typedef std::chrono::system_clock::duration t_dur_sys;

	protected:
		t_tp_sys m_timeStart;
		t_tp_st m_timeStart_st, m_timeStop_st;

		t_dur m_dur;
		t_dur_sys m_dur_sys;

		T m_dDur = T(0);

	public:
		Stopwatch() = default;
		~Stopwatch() = default;

		void start()
		{
			m_timeStart = std::chrono::system_clock::now();
			m_timeStart_st = std::chrono::steady_clock::now();
		}

		void stop()
		{
			m_timeStop_st = std::chrono::steady_clock::now();

			m_dur = std::chrono::duration_cast<t_dur>(m_timeStop_st-m_timeStart_st);
			m_dur_sys = std::chrono::duration_cast<t_dur_sys>(m_dur);

			m_dDur = T(t_dur::period::num)/T(t_dur::period::den) * T(m_dur.count());
		}

		T GetDur() const
		{
			return m_dDur;
		}

		static std::string to_str(const t_tp_sys& t)
		{
			std::time_t tStart = std::chrono::system_clock::to_time_t(t);
			std::tm tmStart = *std::localtime(&tStart);

			char cTime[256];
			std::strftime(cTime, sizeof cTime, "%a %Y-%b-%d %H:%M:%S %Z", &tmStart);
			return std::string(cTime);
		}

		std::string GetStartTimeStr() const { return to_str(m_timeStart); }
		std::string GetStopTimeStr() const { return to_str(m_timeStart+m_dur_sys); }

		t_tp_sys GetEstStopTime(T dProg) const
		{
			t_tp_st timeStop_st = std::chrono::steady_clock::now();
			t_dur dur = std::chrono::duration_cast<t_dur>(timeStop_st - m_timeStart_st);
			dur *= (T(1)/dProg);

			t_dur_sys dur_sys = std::chrono::duration_cast<t_dur_sys>(dur);
			t_tp_sys tpEnd = m_timeStart + dur_sys;
			return tpEnd;
		}

		std::string GetEstStopTimeStr(T dProg) const
		{
			return to_str(GetEstStopTime(dProg));
		}
};


template<class t_real = double>
std::string get_duration_str_secs(t_real dDur)
{
	int iAgeMS = int((dDur - std::trunc(dDur)) * t_real(1000.));
	int iAge[] = { int(dDur), 0, 0, 0 };	// s, m, h, d
	const int iConv[] = { 60, 60, 24 };
	const char* pcUnit[] = { "s ", "m ", "h ", "d " };

	for(std::size_t i=0; i<sizeof(iAge)/sizeof(iAge[0])-1; ++i)
	{
		if(iAge[i] > iConv[i])
		{
			iAge[i+1] = iAge[i] / iConv[i];
			iAge[i] = iAge[i] % iConv[i];
		}
	}

	bool bHadPrev = 0;
	std::string strAge;
	for(std::ptrdiff_t i=sizeof(iAge)/sizeof(iAge[0])-1; i>=0; --i)
	{
		if(iAge[i] || bHadPrev)
		{
			strAge += std::to_string(iAge[i]) + pcUnit[i];
			bHadPrev = 1;
		}
	}
	/*if(iAgeMS)*/ { strAge += std::to_string(iAgeMS) + "ms "; bHadPrev = 1; }

	return strAge;
}


template<class t_real = double>
std::string get_duration_str(const std::chrono::duration<t_real>& dur)
{
	using t_dur = std::chrono::duration<t_real>;

	t_real dDurSecs = t_real(t_dur::period::num)/t_real(t_dur::period::den)
		* t_real(dur.count());
	return get_duration_str_secs(dDurSecs);
}


// ----------------------------------------------------------------------------


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
