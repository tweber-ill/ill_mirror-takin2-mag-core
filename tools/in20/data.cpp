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
