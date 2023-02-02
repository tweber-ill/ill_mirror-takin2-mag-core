/**
 * internal data representation
 * @author Tobias Weber <tweber@ill.fr>
 * @date 1-June-2018
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "data.h"
#include "globals.h"

#include "tlibs2/libs/instr.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/algos.h"



/**
 * convert an instrument data file to the internal data format
 */
std::tuple<bool, Dataset> Dataset::convert_instr_file(const char* pcFile)
{
	Dataset dataset;


	// load instrument data file
	std::shared_ptr<tl2::FileInstrBase<t_real>> pInstr = tl2::FileInstrBase<t_real>::LoadInstr(pcFile);
	if(!pInstr)
		return std::make_tuple(false, dataset);
	const auto &colnames = pInstr->GetColNames();
	const auto &filedata = pInstr->GetData();

	if(!pInstr || !colnames.size())	// only valid files with a non-zero column count
		return std::make_tuple(false, dataset);


	// process polarisation data
	pInstr->SetPolNames("p1", "p2", "i11", "i10");
	pInstr->ParsePolData();


	// get scan axis indices
	std::vector<std::size_t> scan_idx;
	for(const auto& scanvar : pInstr->GetScannedVars())
	{
		std::size_t idx = 0;
		pInstr->GetCol(scanvar, &idx);
		if(idx < colnames.size())
			scan_idx.push_back(idx);
	}
	// try first axis if none found
	if(scan_idx.size() == 0) scan_idx.push_back(0);


	// get counter column index
	std::vector<std::size_t> ctr_idx;
	{
		std::size_t idx = 0;
		pInstr->GetCol(pInstr->GetCountVar(), &idx);
		if(idx < colnames.size())
			ctr_idx.push_back(idx);
	}
	// try second axis if none found
	if(ctr_idx.size() == 0) ctr_idx.push_back(1);


	// get monitor column index
	std::vector<std::size_t> mon_idx;
	{
		std::size_t idx = 0;
		pInstr->GetCol(pInstr->GetMonVar(), &idx);
		if(idx < colnames.size())
			mon_idx.push_back(idx);
	}


	std::size_t numpolstates = pInstr->NumPolChannels();
	if(numpolstates == 0) numpolstates = 1;



	// iterate through all (polarisation) subplots
	for(std::size_t polstate=0; polstate<numpolstates; ++polstate)
	{
		Data data;

		// get scan axes
		for(std::size_t idx : scan_idx)
		{
			std::vector<t_real> thedat;
			tl2::copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			data.AddAxis(thedat, colnames[idx]);
		}


		// get counters
		for(std::size_t idx : ctr_idx)
		{
			std::vector<t_real> thedat, theerr;
			tl2::copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			std::transform(thedat.begin(), thedat.end(), std::back_inserter(theerr),
				[](t_real y) -> t_real
				{
					if(tl2::equals<t_real>(y, 0))
						return 1;
					return std::sqrt(y);
				});

			data.AddCounter(thedat, theerr);
		}


		// get monitors
		for(std::size_t idx : mon_idx)
		{
			std::vector<t_real> thedat, theerr;
			tl2::copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			std::transform(thedat.begin(), thedat.end(), std::back_inserter(theerr),
				[](t_real y) -> t_real
				{
					if(tl2::equals<t_real>(y, 0))
						return 1;
					return std::sqrt(y);
				});

			data.AddMonitor(thedat, theerr);
		}

		dataset.AddChannel(std::move(data));
	}


	return std::make_tuple(true, dataset);
}





// ----------------------------------------------------------------------------
// data operators
// ----------------------------------------------------------------------------

Data Data::add_pointwise(const Data& dat1, const Data& dat2)
{
	// check if x axes and dimensions are equal
	bool compatible = true;
	compatible = compatible && (dat1.m_x_names == dat2.m_x_names);
	compatible = compatible && (dat1.m_x.size() == dat2.m_x.size());
	if(compatible)
	{
		for(std::size_t i=0; i<dat2.m_x.size(); ++i)
		{
			if(!tl2::equals(dat1.m_x[i], dat2.m_x[i], g_eps_merge))
			{
				compatible = false;
				break;
			}
		}
	}

	if(!compatible)
	{
		print_err("Cannot add incompatible data sets: x axes do not match.");
		return Data();
	}


	Data datret = dat1;

	// detectors
	for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
	{
		auto& det = datret.m_counts[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt += dat2.m_counts[detidx][cntidx];

			datret.m_counts_err[detidx][cntidx] =
				std::sqrt(dat1.m_counts_err[detidx][cntidx]*dat1.m_counts_err[detidx][cntidx]
					+ dat2.m_counts_err[detidx][cntidx]*dat2.m_counts_err[detidx][cntidx]);
		}
	}

	// monitors
	for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
	{
		auto& det = datret.m_monitors[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt += dat2.m_monitors[detidx][cntidx];

			datret.m_monitors_err[detidx][cntidx] =
				std::sqrt(dat1.m_monitors_err[detidx][cntidx]*dat1.m_monitors_err[detidx][cntidx]
					+ dat2.m_monitors_err[detidx][cntidx]*dat2.m_monitors_err[detidx][cntidx]);
		}
	}

	return datret;
}


/**
 * merge data sets
 */
Data Data::merge(const Data& dat1, const Data& dat2)
{
	Data datret = dat1;


	// find index in datret, where the x values match with dat2
	auto get_col_idx = [&datret, &dat2](std::size_t idx2) -> std::optional<std::size_t>
	{
		std::optional<std::size_t> idx;

		for(std::size_t x1=0; x1<datret.GetNumAxes(); ++x1)
		{
			const std::string& name_x1 = datret.GetAxisName(x1);

			// find matching axis in dat2
			auto iter2 = std::find(dat2.m_x_names.begin(), dat2.m_x_names.end(), name_x1);
			if(iter2 == dat2.m_x_names.end())
				return std::nullopt;

			std::size_t x2 = iter2 - dat2.m_x_names.begin();

			t_real_dat xval2 = dat2.GetAxis(x2)[idx2];
			const auto& xvals1 = datret.GetAxis(x1);

			if(idx)
			{
				// already found the x index
				if(!tl2::equals<t_real_dat>(xvals1[*idx], xval2, g_eps_merge))
					return std::nullopt;
			}
			else
			{
				// find the position on the datret axis, which has the same value as the given one on dat2
				auto iter = std::find_if(xvals1.begin(), xvals1.end(),
					[xval2](t_real_dat xval1) -> bool
				{
					return tl2::equals<t_real_dat>(xval1, xval2, g_eps_merge);
				});

				if(iter == xvals1.end())
					return std::nullopt;

				idx = iter - xvals1.begin();
			}
		}

		return idx;
	};


	// iterate columns of the scan
	for(std::size_t cntidx2=0; cntidx2<dat2.GetNumCounts(); ++cntidx2)
	{
		// find index in datret, where the x values are the same
		std::optional<std::size_t> cntidx = get_col_idx(cntidx2);

		if(cntidx)
		{
			// if the same point exists in datret, merge them

			// merge detectors
			for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
			{
				auto& cnt = datret.m_counts[detidx][*cntidx];
				cnt += dat2.m_counts[detidx][cntidx2];

				datret.m_counts_err[detidx][*cntidx] =
					std::sqrt(datret.m_counts_err[detidx][*cntidx]*datret.m_counts_err[detidx][*cntidx]
						+ dat2.m_counts_err[detidx][cntidx2]*dat2.m_counts_err[detidx][cntidx2]);
			}

			// merge monitors
			for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
			{
				auto& cnt = datret.m_monitors[detidx][*cntidx];
				cnt += dat2.m_monitors[detidx][cntidx2];

				datret.m_monitors_err[detidx][*cntidx] =
					std::sqrt(datret.m_monitors_err[detidx][*cntidx]*datret.m_monitors_err[detidx][*cntidx]
						+ dat2.m_monitors_err[detidx][cntidx2]*dat2.m_monitors_err[detidx][cntidx2]);
			}
		}
		else
		{
			// if the same point does not exist in datret, append it

			// append x axes
			for(std::size_t xidx=0; xidx<std::min(datret.m_x_names.size(), datret.m_x.size()); ++xidx)
			{
				// find matching axis
				auto iter2 = std::find(dat2.m_x_names.begin(), dat2.m_x_names.end(), datret.m_x_names[xidx]);
				if(iter2 == dat2.m_x_names.end())
					continue;

				// insert data
				std::size_t xidx2 = iter2 - dat2.m_x_names.begin();
				datret.m_x[xidx].push_back(dat2.m_x[xidx2][cntidx2]);
			}

			// append detectors
			for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
			{
				datret.m_counts[detidx].push_back(dat2.m_counts[detidx][cntidx2]);
				datret.m_counts_err[detidx].push_back(std::sqrt(dat2.m_counts[detidx][cntidx2]));
			}

			// append monitors
			for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
			{
				datret.m_monitors[detidx].push_back(dat2.m_monitors[detidx][cntidx2]);
				datret.m_monitors_err[detidx].push_back(std::sqrt(dat2.m_monitors[detidx][cntidx2]));
			}
		}
	}

	return datret;
}


/**
 * append dat2 to the end of dat1
 */
Data Data::append(const Data& dat1, const Data& dat2)
{
	if(dat1.m_counts.size() != dat2.m_counts.size())
	{
		print_err("Mismatch in number of detector counters.");
		return Data();
	}

	if(dat1.m_monitors.size() != dat2.m_monitors.size())
	{
		print_err("Mismatch in number of monitor counters.");
		return Data();
	}


	Data datret = dat1;

	// append x axes
	for(std::size_t xidx=0; xidx<std::min(dat1.m_x_names.size(), dat1.m_x.size()); ++xidx)
	{
		//std::cout << "Appending column " << dat1.m_x_names[xidx] << std::endl;

		// find matching axis
		auto iter2 = std::find(dat2.m_x_names.begin(), dat2.m_x_names.end(), dat1.m_x_names[xidx]);
		if(iter2 == dat2.m_x_names.end())
		{
			print_err("Column \"", dat1.m_x_names[xidx], "\" was not found in all data sets. Ignoring.");
			continue;
		}

		// insert data
		std::size_t xidx2 = iter2 - dat2.m_x_names.begin();
		datret.m_x[xidx].insert(datret.m_x[xidx].end(), dat2.m_x[xidx2].begin(), dat2.m_x[xidx2].end());
	}

	// append counters, monitors, and their errors
	for(std::size_t yidx=0; yidx<dat1.m_counts.size(); ++yidx)
		datret.m_counts[yidx].insert(datret.m_counts[yidx].end(), dat2.m_counts[yidx].begin(), dat2.m_counts[yidx].end());
	for(std::size_t yidx=0; yidx<dat1.m_counts_err.size(); ++yidx)
		datret.m_counts_err[yidx].insert(datret.m_counts_err[yidx].end(), dat2.m_counts_err[yidx].begin(), dat2.m_counts_err[yidx].end());
	for(std::size_t yidx=0; yidx<dat1.m_monitors.size(); ++yidx)
		datret.m_monitors[yidx].insert(datret.m_monitors[yidx].end(), dat2.m_monitors[yidx].begin(), dat2.m_monitors[yidx].end());
	for(std::size_t yidx=0; yidx<dat1.m_counts_err.size(); ++yidx)
		datret.m_monitors_err[yidx].insert(datret.m_monitors_err[yidx].end(), dat2.m_monitors_err[yidx].begin(), dat2.m_monitors_err[yidx].end());

	return datret;
}


const Data& operator +(const Data& dat)
{
	return dat;
}


Data operator -(const Data& dat)
{
	Data datret = dat;

	// detectors
	for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
	{
		auto& det = datret.m_counts[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt = -cnt;
		}
	}

	// monitors
	for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
	{
		auto& det = datret.m_monitors[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt = -cnt;
		}
	}

	return datret;
}


Data operator +(const Data& dat1, const Data& dat2)
{
	return Data::add_pointwise(dat1, dat2);
}


Data operator +(const Data& dat, t_real_dat d)
{
	Data datret = dat;
	t_real_dat d_err = std::sqrt(d);
	t_real_dat d_mon = 0.;	// TODO
	t_real_dat d_mon_err = std::sqrt(d_mon);

	// detectors
	for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
	{
		auto& det = datret.m_counts[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt += d;

			datret.m_counts_err[detidx][cntidx] =
				std::sqrt(dat.m_counts_err[detidx][cntidx]*dat.m_counts_err[detidx][cntidx]
					+ d*d_err);
		}
	}

	// monitors
	for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
	{
		auto& det = datret.m_monitors[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			auto& cnt = det[cntidx];
			cnt += d_mon;

			datret.m_monitors_err[detidx][cntidx] =
				std::sqrt(dat.m_monitors_err[detidx][cntidx]*dat.m_monitors_err[detidx][cntidx]
					+ d_mon*d_mon_err);
		}
	}

	return datret;
}


Data operator +(t_real_dat d, const Data& dat)
{
	return dat + d;
}


Data operator -(const Data& dat, t_real_dat d)
{
	return dat + (-d);
}


Data operator -(const Data& dat1, const Data& dat2)
{
	return dat1 + (-dat2);
}


Data operator *(const Data& dat1, t_real_dat d)
{
	Data datret = dat1;

	// detectors
	for(std::size_t detidx=0; detidx<datret.m_counts.size(); ++detidx)
	{
		auto& det = datret.m_counts[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			det[cntidx] *= d;
			datret.m_counts_err[detidx][cntidx] = d*dat1.m_counts_err[detidx][cntidx];
		}
	}

	// monitors
	for(std::size_t detidx=0; detidx<datret.m_monitors.size(); ++detidx)
	{
		auto& det = datret.m_monitors[detidx];

		for(std::size_t cntidx=0; cntidx<det.size(); ++cntidx)
		{
			det[cntidx] *= d;
			datret.m_monitors_err[detidx][cntidx] = d*dat1.m_monitors_err[detidx][cntidx];
		}
	}

	return datret;
}


Data operator *(t_real_dat d, const Data& dat1)
{
	return dat1 * d;
}


Data operator /(const Data& dat1, t_real d)
{
	return dat1 * (t_real(1)/d);
}



/**
 * normalise to monitor counter
 */
Data Data::norm(std::size_t monidx) const
{
	if(GetNumCounters() != GetNumMonitors())
	{
		print_err("Number of monitors has to be equal to the number of detector counters.");
		return *this;
	}
	if(monidx >= GetNumMonitors())
	{
		print_err("Invalid monitor selected.");
		return *this;
	}


	Data datret = *this;

	// normalise all counters
	for(std::size_t detidx=0; detidx<GetNumCounters(); ++detidx)
	{
		const auto& det = GetCounter(detidx);
		const auto& deterr = GetCounterErrors(detidx);
		const auto& mon = GetMonitor(monidx);
		const auto& monerr = GetMonitorErrors(monidx);

		if(det.size()!=deterr.size() || det.size()!=mon.size() || det.size()!=monerr.size())
		{
			print_err("Data, monitor and error columns have to be of equal size."
				" [det=", det.size(), " deterr=", deterr.size(), " mon=", mon.size(), ", monerr=", monerr.size(), "]");
			return *this;
		}

		// newcnts = cnts/mon
		for(std::size_t pt=0; pt<det.size(); ++pt)
		{
			datret.m_counts[detidx][pt] = det[pt] / mon[pt];
			datret.m_counts_err[detidx][pt] = std::sqrt(std::pow(deterr[pt]/mon[pt], 2) + std::pow(-monerr[pt]*det[pt]/(mon[pt]*mon[pt]), 2));
		}
	}


	// normalise monitor with itself
	for(std::size_t pt=0; pt<GetMonitor(monidx).size(); ++pt)
	{
		datret.m_monitors[monidx][pt] = 1.;
		datret.m_monitors_err[monidx][pt] = 0.;
	}

	return datret;
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// dataset operators
// ----------------------------------------------------------------------------

Dataset Dataset::add_pointwise(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<std::min(dat1.GetNumChannels(), dat2.GetNumChannels()); ++ch)
	{
		Data dat = Data::add_pointwise(dat1.GetChannel(ch), dat2.GetChannel(ch));
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


Dataset Dataset::merge(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<std::min(dat1.GetNumChannels(), dat2.GetNumChannels()); ++ch)
	{
		Data dat = Data::merge(dat1.GetChannel(ch), dat2.GetChannel(ch));
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


Dataset Dataset::append(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<std::min(dat1.GetNumChannels(), dat2.GetNumChannels()); ++ch)
	{
		Data dat = Data::append(dat1.GetChannel(ch), dat2.GetChannel(ch));
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


Dataset Dataset::append_channels(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset = dat1;

	for(std::size_t ch2=0; ch2<dat2.GetNumChannels(); ++ch2)
	{
		const Data& thechannel= dat2.GetChannel(ch2);
		dataset.AddChannel(thechannel);
	}

	return dataset;
}


Dataset operator +(const Dataset& dat1, const Dataset& dat2)
{
	return Dataset::add_pointwise(dat1, dat2);
}


Dataset operator -(const Dataset& dat1, const Dataset& dat2)
{
	return dat1 + (-dat2);
}


Dataset operator +(const Dataset& dat, t_real_dat d)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<dat.GetNumChannels(); ++ch)
	{
		Data data = dat.GetChannel(ch) + d;
		dataset.AddChannel(std::move(data));
	}

	return dataset;
}


Dataset operator +(t_real_dat d, const Dataset& dat)
{
	return dat + d;
}


Dataset operator -(const Dataset& dat, t_real_dat d)
{
	return dat + (-d);
}


Dataset operator *(const Dataset& dat1, t_real_dat d)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<dat1.GetNumChannels(); ++ch)
	{
		Data dat = dat1.GetChannel(ch) * d;
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


Dataset operator *(t_real_dat d, const Dataset& dat1)
{
	return operator *(dat1, d);
}


Dataset operator /(const Dataset& dat1, t_real_dat d)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<dat1.GetNumChannels(); ++ch)
	{
		Data dat = dat1.GetChannel(ch) / d;
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


const Dataset& operator +(const Dataset& dat)
{
	return dat;
}


Dataset operator -(const Dataset& dat1)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<dat1.GetNumChannels(); ++ch)
	{
		Data dat = -dat1.GetChannel(ch);
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}



/**
 * normalise to monitor counter
 */
Dataset Dataset::norm(std::size_t mon) const
{
	Dataset dataset;

	for(std::size_t ch=0; ch<GetNumChannels(); ++ch)
		dataset.AddChannel(GetChannel(ch).norm(mon));

	return dataset;
}



void Dataset::clear()
{
	m_data.clear();
}


/**
 * export data to gnuplot
 */
bool Dataset::SaveGpl(const std::string& file) const
{
	std::ofstream ofstr(file);
	if(!ofstr)
		return false;

	t_real prec_scale = 1.75;
	ofstr.precision(g_prec);


	// colours
	ofstr << "col_0 = \"#ff0000\"\n";
	ofstr << "col_1 = \"#0000ff\"\n";
	ofstr << "col_2 = \"#009900\"\n";
	ofstr << "col_3 = \"#000000\"\n\n\n";

	const std::string cols[] = { "col_0", "col_1", "col_2", "col_3" };
	const std::size_t iNumCols = sizeof(cols) / sizeof(*cols);

	const int pts[] = {7, 5, 9, 11, 13};
	const std::size_t iNumPts = sizeof(pts) / sizeof(*pts);


	std::string xlabel = "";

	std::size_t N = GetNumChannels();
	for(std::size_t ch=0; ch<N; ++ch)
	{
		const Data& dat = GetChannel(ch);
		const std::size_t Nx = dat.GetNumAxes();
		const std::size_t Nc = dat.GetNumCounters();
		const std::size_t Nm = dat.GetNumMonitors();

		// channel empty ?
		if(Nx == 0) continue;


		// has to be the same for all columns
		std::size_t rows = dat.GetCounter(0).size();

		// take overall x label from first channel
		if(xlabel == "" && Nx >= 1)
			xlabel = dat.GetAxisName(0);

		ofstr << "$dat_" << ch << " << ENDDATA\n";

		// iterate all x axis names
		ofstr << "# ";
		for(std::size_t iX = 0; iX < Nx; ++iX)
			ofstr << std::setw(iX==0 ? g_prec*prec_scale - 2 : g_prec*prec_scale) << std::left << dat.GetAxisName(iX) << " ";

		// iterate all counter names
		for(std::size_t iC = 0; iC < Nc; ++iC)
			ofstr << std::setw(g_prec*prec_scale) << std::left << "counter " << " "
				<< std::setw(g_prec*prec_scale) << std::left << "error" << " ";

		// iterate all monitors names
		for(std::size_t iM = 0; iM < Nm; ++iM)
			ofstr << std::setw(g_prec*prec_scale) << std::left << "monitor " << " "
				<< std::setw(g_prec*prec_scale) << std::left << "error" << " ";

		ofstr << "\n";

		// find x sorting
		std::vector<std::size_t> rows_sorted = tl2::get_perm(rows,
			[&dat](std::size_t idx1, std::size_t idx2) -> bool
			{
				return dat.GetAxis(0)[idx1] < dat.GetAxis(0)[idx2];
			});

		// iterate data rows
		for(std::size_t row_idx : rows_sorted)
		{
			// iterate all x axes
			for(std::size_t iX = 0; iX < Nx; ++iX)
			{
				const auto& vec = dat.GetAxis(iX);
				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " ";
			}


			// iterate all counters
			for(std::size_t iC = 0; iC < Nc; ++iC)
			{
				const auto& vec = dat.GetCounter(iC);
				const auto& vecErr = dat.GetCounterErrors(iC);

				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " "
					<< std::setw(g_prec*prec_scale) << std::left << vecErr[row_idx] << " ";
			}


			// iterate all monitors
			for(std::size_t iM = 0; iM < Nm; ++iM)
			{
				const auto& vec = dat.GetMonitor(iM);
				const auto& vecErr = dat.GetMonitorErrors(iM);

				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " "
					<< std::setw(g_prec*prec_scale) << std::left << vecErr[row_idx];
			}


			ofstr << "\n";
		}

		ofstr << "ENDDATA\n\n";
	}


	ofstr << "\nset xlabel \"" << xlabel << "\"\n";
	ofstr << "set ylabel \"Counts\"\n\n";

	ofstr << "\nplot \\\n";
	for(std::size_t ch=0; ch<N; ++ch)
	{
		const Data& dat = GetChannel(ch);
		const std::size_t Nx = dat.GetNumAxes();
		const std::size_t idx_x = 1;
		const std::size_t idx_y = Nx+1;
		const std::size_t idx_yerr = idx_y+1;

		ofstr << "\t$dat_" << ch << " u ($" << idx_x << "):($" << idx_y << "):($" << idx_yerr << ")"
			<< " w yerrorbars pt " << pts[ch % iNumPts]
			<< " ps 1 lw 1.5 lc rgb " << cols[ch % iNumCols]
			<< " title \"Channel " << (ch+1) << "\""
			<< (ch == N-1 ? "\n" : ", \\\n");
	}

	return true;
}


/**
 * export to data file
 */
bool Dataset::Save(const std::string& file) const
{
	std::ofstream ofstr(file);
	if(!ofstr)
		return false;

	t_real prec_scale = 1.75;
	ofstr.precision(g_prec);

	// write a header stub
	ofstr << "#\n";
	ofstr << "# sample_a = 0\n";
	ofstr << "# sample_b = 0\n";
	ofstr << "# sample_c = 0\n";
	ofstr << "# sample_alpha = 90\n";
	ofstr << "# sample_beta = 90\n";
	ofstr << "# sample_gamma = 90\n";
	ofstr << "# orient1_x = 1\n";
	ofstr << "# orient1_y = 0\n";
	ofstr << "# orient1_z = 0\n";
	ofstr << "# orient2_x = 0\n";
	ofstr << "# orient2_y = 1\n";
	ofstr << "# orient2_z = 0\n";
	ofstr << "# mono_d = 0\n";
	ofstr << "# ana_d = 0\n";
	ofstr << "# sense_m = 1\n";
	ofstr << "# sense_s = -1\n";
	ofstr << "# sense_a = 1\n";
	ofstr << "# k_fix = 1.4\n";
	ofstr << "# is_ki_fixed = 0\n";
	ofstr << "# col_h = 0\n";
	ofstr << "# col_k = 0\n";
	ofstr << "# col_l = 0\n";
	ofstr << "# col_E = 1\n";
	ofstr << "# cols_scanned = 1\n";
	ofstr << "# col_ctr = 2\n";
	ofstr << "# col_ctr_err = 3\n";
	ofstr << "# col_mon = 4\n";
	ofstr << "# col_mon_err = 5\n";
	ofstr << "#\n";

	std::size_t N = GetNumChannels();
	for(std::size_t ch=0; ch<N; ++ch)
	{
		const Data& dat = GetChannel(ch);
		const std::size_t Nx = dat.GetNumAxes();
		const std::size_t Nc = dat.GetNumCounters();
		const std::size_t Nm = dat.GetNumMonitors();

		// channel empty ?
		if(Nx == 0) continue;

		// has to be the same for all columns
		std::size_t rows = dat.GetCounter(0).size();

		// iterate all x axis names
		ofstr << "# ";
		for(std::size_t iX = 0; iX < Nx; ++iX)
			ofstr << std::setw(iX==0 ? g_prec*prec_scale - 2 : g_prec*prec_scale) << std::left << dat.GetAxisName(iX) << " ";

		// iterate all counter names
		for(std::size_t iC = 0; iC < Nc; ++iC)
			ofstr << std::setw(g_prec*prec_scale) << std::left << "counter " << " "
				<< std::setw(g_prec*prec_scale) << std::left << "error" << " ";

		// iterate all monitors names
		for(std::size_t iM = 0; iM < Nm; ++iM)
			ofstr << std::setw(g_prec*prec_scale) << std::left << "monitor " << " "
				<< std::setw(g_prec*prec_scale) << std::left << "error" << " ";
		ofstr << "\n";

		// find x sorting
		std::vector<std::size_t> rows_sorted = tl2::get_perm(rows,
			[&dat](std::size_t idx1, std::size_t idx2) -> bool
			{
				return dat.GetAxis(0)[idx1] < dat.GetAxis(0)[idx2];
			});

		// iterate data rows
		for(std::size_t row_idx : rows_sorted)
		{
			// iterate all x axes
			for(std::size_t iX = 0; iX < Nx; ++iX)
			{
				const auto& vec = dat.GetAxis(iX);
				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " ";
			}

			// iterate all counters
			for(std::size_t iC = 0; iC < Nc; ++iC)
			{
				const auto& vec = dat.GetCounter(iC);
				const auto& vecErr = dat.GetCounterErrors(iC);

				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " "
					<< std::setw(g_prec*prec_scale) << std::left << vecErr[row_idx] << " ";
			}

			// iterate all monitors
			for(std::size_t iM = 0; iM < Nm; ++iM)
			{
				const auto& vec = dat.GetMonitor(iM);
				const auto& vecErr = dat.GetMonitorErrors(iM);

				ofstr << std::setw(g_prec*prec_scale) << std::left << vec[row_idx] << " "
					<< std::setw(g_prec*prec_scale) << std::left << vecErr[row_idx];
			}

			ofstr << "\n";
		}
	}

	return true;
}
// ----------------------------------------------------------------------------
