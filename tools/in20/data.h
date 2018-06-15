/**
 * internal data representation
 * @author Tobias Weber <tweber@ill.fr>
 * @date 1-June-2018
 * @license see 'LICENSE' file
 */

#ifndef __DATAREP_H__
#define __DATAREP_H__

#include <vector>
#include <string>
#include <tuple>

using t_real_dat = double;



/**
 * data set (e.g. of one polarisation channel)
 */
class Data
{
private:
	// counts
	// can have multiple detectors and monitors
	std::vector<std::vector<t_real_dat>> m_counts;
	std::vector<std::vector<t_real_dat>> m_counts_err;

	// monitors
	std::vector<std::vector<t_real_dat>> m_monitors;
	std::vector<std::vector<t_real_dat>> m_monitors_err;

	// x axes
	std::vector<std::vector<t_real_dat>> m_x;
	std::vector<std::string> m_x_names;

public:
	std::size_t GetNumCounters() const { return m_counts.size(); }
	std::size_t GetNumMonitors() const { return m_monitors.size(); }
	std::size_t GetNumAxes() const { return m_x.size(); }


	// counters
	const std::vector<t_real_dat>& GetCounter(std::size_t i) const { return m_counts[i]; }
	const std::vector<t_real_dat>& GetCounterErrors(std::size_t i) const { return m_counts_err[i]; }
	void AddCounter(const std::vector<t_real_dat> &dat, const std::vector<t_real_dat> &err)
	{
		m_counts.push_back(dat);
		m_counts_err.push_back(err);
	}
	void AddCounter(std::vector<t_real_dat> &&dat, std::vector<t_real_dat> &&err)
	{
		m_counts.push_back(std::move(dat));
		m_counts_err.push_back(std::move(err));
	}


	// monitors
	const std::vector<t_real_dat>& GetMonitor(std::size_t i) const { return m_monitors[i]; }
	const std::vector<t_real_dat>& GetMonitorErrors(std::size_t i) const { return m_monitors_err[i]; }
	void AddMonitor(const std::vector<t_real_dat> &dat, const std::vector<t_real_dat> &err)
	{
		m_monitors.push_back(dat);
		m_monitors_err.push_back(err);
	}
	void AddMonitor(std::vector<t_real_dat> &&dat, std::vector<t_real_dat> &&err)
	{
		m_monitors.push_back(std::move(dat));
		m_monitors_err.push_back(std::move(err));
	}


	// x axes
	const std::vector<t_real_dat>& GetAxis(std::size_t i) const { return m_x[i]; }
	const std::string& GetAxisName(std::size_t i) const { return m_x_names[i]; }
	void AddAxis(const std::vector<t_real_dat> &dat, const std::string &name="")
	{
		m_x.push_back(dat);

		if(name != "")
			m_x_names.push_back(name);
		else
			m_x_names.push_back("ax" + std::to_string(GetNumAxes()));
	}
	void AddAxis(std::vector<t_real_dat> &&dat, const std::string &name="")
	{
		m_x.emplace_back(std::move(dat));

		if(name != "")
			m_x_names.push_back(name);
		else
			m_x_names.push_back("ax" + std::to_string(GetNumAxes()));
	}
};



/**
 * collection of individual data (i.e. polarisation channels)
 */
class Dataset
{
private:
	std::vector<Data> m_data;

public:
	std::size_t GetNumChannels() const { return m_data.size(); }
	const Data& GetChannel(std::size_t channel) const { return m_data[channel]; }
	void AddChannel(const Data &data) { m_data.push_back(data); }
	void AddChannel(Data &&data) { m_data.emplace_back(std::move(data)); }


	static std::tuple<bool, Dataset> convert_instr_file(const char* pcFile);
};


#endif