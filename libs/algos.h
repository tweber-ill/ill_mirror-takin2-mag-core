/**
 * tlibs2
 * algorithm library
 * @author Tobias Weber <tweber@ill.fr>
 * @date Apr-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_ALGOS_H__
#define __TLIBS2_ALGOS_H__

#include <algorithm>
#include <string>
#include <chrono>

namespace tl2 {

/**
 * copy algorithm with interleave
 */
template<class T1, class T2>
void copy_interleave(T1 inIter, T1 inEnd, T2 outIter, std::size_t interleave, std::size_t startskip)
{
	std::advance(inIter, startskip);

	while(std::distance(inIter, inEnd) > 0)
	{
		*outIter = *inIter;

		++outIter;
		std::advance(inIter, interleave);
	}
}


/**
 * count number of ocurrences of a sub-string in a string
 */
static std::size_t count_occurrences(const std::string &str, const std::string &tok)
{
	std::size_t num = 0;
	std::size_t start = 0;
	const std::size_t len_tok = tok.length();

	while(true)
	{
		std::size_t idx = str.find(tok, start);
		if(idx == std::string::npos)
			break;

		++num;
		start += idx+len_tok;
	}

	return num;
}


/**
 * merge containers
 */
template<class t_cont>
t_cont arrayunion(const std::initializer_list<t_cont>& lst)
{
	t_cont contRet;
	for(const t_cont& cont : lst)
		contRet.insert(contRet.end(), cont.begin(), cont.end());
	return contRet;
}


/**
 * minimum of four numbers
 */
template<typename T=double>
T min4(T t1, T t2, T t3, T t4)
{
	T tmin = t1;
	tmin = std::min(tmin, t2);
	tmin = std::min(tmin, t3);
	tmin = std::min(tmin, t4);
	return tmin;
}



/**
 * like std::chrono::seconds/minutes/hours, but with variable type
 */
template<typename T = long >
using t_dur_secs = std::chrono::duration<T, std::ratio<1, 1>>;
template<typename T = long >
using t_dur_mins = std::chrono::duration<T, std::ratio<60, 1>>;
template<typename T = long >
using t_dur_hours = std::chrono::duration<T, std::ratio<60*60, 1>>;

template<typename T = long >
using t_dur_days = std::chrono::duration<T, std::ratio<60*60*24, 1>>;

template<typename T = long >
using t_dur_weeks = std::chrono::duration<T, std::ratio<60*60*24*7, 1>>;


/**
 * duration since epoch
 */
template<typename t_dur = std::chrono::seconds>
t_dur epoch_dur()
{
	namespace ch = std::chrono;
	return ch::duration_cast<t_dur>(ch::system_clock::now().time_since_epoch());
}


/**
 * seconds since epoch
 */
template<typename T=double>
T epoch()
{
	return epoch_dur<t_dur_secs<T>>().count();
}


/**
 * create a string representation of epoch
 */
template<typename T=double>
std::string epoch_to_str(T tSeconds, const char *pcFmt="%a %Y-%b-%d %H:%M:%S %Z")
{
	namespace ch = std::chrono;

	t_dur_secs<T> secs(tSeconds);
	ch::system_clock::time_point tp(ch::duration_cast<ch::seconds>(secs));

	std::time_t t = ch::system_clock::to_time_t(tp);
	std::tm tm = *std::localtime(&t);

	char cTime[256];
	std::strftime(cTime, sizeof cTime, pcFmt, &tm);
	return std::string(cTime);
}

}

#endif
