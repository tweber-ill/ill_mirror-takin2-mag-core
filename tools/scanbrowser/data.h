/**
 * internal data representation
 * @author Tobias Weber <tweber@ill.fr>
 * @date 1-June-2018
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

#ifndef __DATAREP_H__
#define __DATAREP_H__

#include <vector>
#include <string>
#include <tuple>

using t_real_dat = double;


class Data;
class Dataset;


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
	void SetAxisNames(const std::vector<std::string> &names) { m_x_names = names; }
	void SetAxisNames(std::vector<std::string> &&names) { m_x_names = std::move(names); }
	void AddAxis(const std::vector<t_real_dat> &dat, const std::string &name="")
	{
		m_x.push_back(dat);

		if(name != "")
			m_x_names.push_back(name);
		else if(m_x_names.size() < m_x.size())
			m_x_names.push_back("ax" + std::to_string(GetNumAxes()));
	}
	void AddAxis(std::vector<t_real_dat> &&dat, const std::string &name="")
	{
		m_x.emplace_back(std::move(dat));

		if(name != "")
			m_x_names.push_back(name);
		else if(m_x_names.size() < m_x.size())
			m_x_names.push_back("ax" + std::to_string(GetNumAxes()));
	}

public:
	Data norm(std::size_t mon = 0) const;


	// binary operators
	friend Data operator +(const Data& dat1, const Data& dat2);
	friend Data operator +(const Data& dat, t_real_dat d);
	friend Data operator +(t_real_dat d, const Data& dat);
	friend Data operator -(const Data& dat1, const Data& dat2);
	friend Data operator -(const Data& dat, t_real_dat d);
	friend Data operator *(const Data& dat1, t_real_dat d);
	friend Data operator *(t_real_dat d, const Data& dat1);
	friend Data operator /(const Data& dat1, t_real_dat d);

	// unary operators
	friend const Data& operator +(const Data& dat);
	friend Data operator -(const Data& dat);


	// different ways of uniting data containers
	static Data add_pointwise(const Data& dat1, const Data& dat2);
	static Data append(const Data& dat1, const Data& dat2);
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

public:
	Dataset norm(std::size_t mon = 0) const;
	void clear();

	// export to gnuplot
	bool SaveGpl(const std::string& file) const;


	// binary operators
	friend Dataset operator +(const Dataset& dat1, const Dataset& dat2);
	friend Dataset operator +(const Dataset& dat, t_real_dat d);
	friend Dataset operator +(t_real_dat d, const Dataset& dat);
	friend Dataset operator -(const Dataset& dat1, const Dataset& dat2);
	friend Dataset operator -(const Dataset& dat, t_real_dat d);
	friend Dataset operator *(const Dataset& dat1, t_real_dat d);
	friend Dataset operator *(t_real_dat d, const Dataset& dat1);
	friend Dataset operator /(const Dataset& dat1, t_real_dat d);

	// unary operators
	friend const Dataset& operator +(const Dataset& dat);
	friend Dataset operator -(const Dataset& dat);


	// different ways of uniting data sets
	static Dataset add_pointwise(const Dataset& dat1, const Dataset& dat2);
	static Dataset append(const Dataset& dat1, const Dataset& dat2);
	static Dataset append_channels(const Dataset& dat1, const Dataset& dat2);

	static std::tuple<bool, Dataset> convert_instr_file(const char* pcFile);
};


#endif
