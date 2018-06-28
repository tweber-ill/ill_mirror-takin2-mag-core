/**
 * internal data representation
 * @author Tobias Weber <tweber@ill.fr>
 * @date 1-June-2018
 * @license see 'LICENSE' file
 */

#include "data.h"
#include "tlibs/file/loadinstr.h"
#include "libs/algos.h"


using t_real = t_real_dat;



/**
 * convert an instrument data file to the internal data format
 */
std::tuple<bool, Dataset> Dataset::convert_instr_file(const char* pcFile)
{
	Dataset dataset;


	// load instrument data file
	std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(pcFile));
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
			copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			data.AddAxis(thedat, colnames[idx]);
		}


		// get counters
		for(std::size_t idx : ctr_idx)
		{
			std::vector<t_real> thedat, theerr;
			copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			std::transform(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(theerr),
				[](t_real y) -> t_real
				{
					if(tl::float_equal<t_real>(y, 0))
						return 1;
					return std::sqrt(y);
				});

			data.AddCounter(thedat, theerr);
		}


		// get monitors
		for(std::size_t idx : mon_idx)
		{
			std::vector<t_real> thedat, theerr;
			copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
			std::transform(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(theerr),
				[](t_real y) -> t_real
				{
					if(tl::float_equal<t_real>(y, 0))
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
	// TODO: check if x axes and dimensions are equal!

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
	return dat1 * t_real(1)/d;
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// dataset operators
// ----------------------------------------------------------------------------

Dataset operator +(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<std::min(dat1.GetNumChannels(), dat2.GetNumChannels()); ++ch)
	{
		Data dat = dat1.GetChannel(ch) + dat2.GetChannel(ch);
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
}


Dataset operator -(const Dataset& dat1, const Dataset& dat2)
{
	Dataset dataset;

	for(std::size_t ch=0; ch<std::min(dat1.GetNumChannels(), dat2.GetNumChannels()); ++ch)
	{
		Data dat = dat1.GetChannel(ch) - dat2.GetChannel(ch);
		dataset.AddChannel(std::move(dat));
	}

	return dataset;
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

// ----------------------------------------------------------------------------
