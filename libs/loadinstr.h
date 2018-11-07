/**
 * tlibs2
 * instrument-file library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date Feb-2015 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_LOADINSTR_H__
#define __TLIBS2_LOADINSTR_H__

#include <unordered_map>
#include <map>
#include <vector>
#include <array>
#include <regex>
#include <numeric>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "string.h"
#include "log.h"
#include "file.h"
#include "math.h"
#include "phys.h"

#include <vector>
#include <unordered_map>


namespace rex = ::std;

namespace tl2{


template<class t_real=double, class t_char=char>
class DatFile
{
	public:
		using t_str = std::basic_string<t_char>;
		using t_col = std::vector<t_real>;
		using t_dat = std::vector<t_col>;
		using t_map = std::unordered_map<t_str, t_str>;

	protected:
		bool m_bOk = 0;

		t_char m_chComm = '#';				// comment
		t_str m_strSeps = {'=', ':'};		// key-value separator
		t_str m_strDatSep = {' ', '\t'};	// data separators

		t_dat m_vecCols;
		std::size_t m_iCurLine = 0;

		t_map m_mapHdr;
		std::vector<t_str> m_vecRawHdr;

	protected:
		void ReadHeaderLine(const t_str& _strLine)
		{
			std::size_t iBeg = _strLine.find_first_not_of(m_chComm);
			if(iBeg == t_str::npos) return;

			t_str strLine(_strLine, iBeg, t_str::npos);
			trim(strLine);
			if(strLine.length() == 0)
				return;
			//log_debug("Header line: ", strLine);

			bool bInsert = 1;
			std::pair<t_str, t_str> pair =
				split_first<t_str>(strLine, t_str({m_strSeps}), 1);
			if(pair.first.length()==0 && pair.second.length()==0)
				bInsert=0;

			if(bInsert)
				m_mapHdr.insert(std::move(pair));
			m_vecRawHdr.emplace_back(std::move(strLine));
		}

		void ReadDataLine(const t_str& strLine)
		{
			std::vector<t_real> vecToks;
			get_tokens<t_real, t_str>(strLine, m_strDatSep, vecToks);
			if(vecToks.size() == 0)
				return;

			if(vecToks.size() > m_vecCols.size())
			{
				if(m_vecCols.size() == 0)	// first line with data
				{
					m_vecCols.resize(vecToks.size());
				}
				else				// add zero columns
				{
					std::size_t iRest = vecToks.size()-m_vecCols.size();
					for(std::size_t iCol=0; iCol<iRest; ++iCol)
					{
						t_col vecNew(m_vecCols[0].size(), t_real(0));
						m_vecCols.emplace_back(std::move(vecNew));
					}

					log_warn("Data file loader: Too many elements in line ", m_iCurLine, ".");
				}
			}
			if(m_vecCols.size() < vecToks.size())
				log_warn("Data file loader: Too few elements in line ", m_iCurLine, ".");

			for(std::size_t iCol=0; iCol<vecToks.size(); ++iCol)
				m_vecCols[iCol].push_back(vecToks[iCol]);
			// fill with 0 if less tokens than columns
			for(std::size_t iCol=vecToks.size(); iCol<m_vecCols.size(); ++iCol)
				m_vecCols[iCol].push_back(t_real(0));
		}

	public:
		void Unload()
		{
			m_bOk = 0;
			m_iCurLine = 0;
			m_vecRawHdr.clear();
			m_mapHdr.clear();
			m_vecCols.clear();
		}

		bool Save(std::basic_ostream<t_char>& ostr)
		{
			if(!m_bOk) return 0;

			for(const typename t_map::value_type& val : m_mapHdr)
				ostr << m_chComm << " " <<
					val.first << " " << m_strSeps[0] << " " << val.second << "\n";

			std::size_t iRows = GetRowCount();
			std::size_t iCols = GetColumnCount();

			for(std::size_t iRow=0; iRow<iRows; ++iRow)
			{
				for(std::size_t iCol=0; iCol<iCols; ++iCol)
					ostr << std::setw(16) << m_vecCols[iCol][iRow] << m_strDatSep[0];
				ostr << "\n";
			}
			return 1;
		}

		bool Save(const t_str& strFile)
		{
			std::ofstream ofstr(strFile);
			if(!ofstr) return 0;
			return Save(ofstr);
		}

		bool Load(std::basic_istream<t_char>& istr)
		{
			Unload();
			while(!istr.eof())
			{
				t_str strLine;
				std::getline(istr, strLine);
				++m_iCurLine;
				trim<t_str>(strLine);
				if(strLine.length() == 0)
					continue;
				//log_debug("Line: ", strLine);

				if(strLine[0] == m_chComm)
					ReadHeaderLine(strLine);
				else
					ReadDataLine(strLine);
			}

			m_bOk = 1;
			return m_bOk;
		}

		bool Load(const t_str& strFile)
		{
			m_bOk = 0;
			//log_debug("File: ", strFile);
			std::basic_ifstream<t_char> ifstr(wstr_to_str(strFile));
			if(!ifstr)
				return false;
			std::basic_istream<t_char> *pIstr = &ifstr;
			return Load(*pIstr);
		}

		bool IsOk() const { return m_bOk; }
		operator bool() const { return IsOk(); }

		void SetCommentChar(t_char ch) { m_chComm = ch; }
		void SetSeparatorChars(const t_str& str) { m_strSeps = str; }
		void SetDataSeparators(const t_str& str) { m_strDatSep = str; };

		const t_dat& GetData() const { return m_vecCols; }
		t_dat& GetData() { return m_vecCols; }
		const t_col& GetColumn(std::size_t iCol) const { return m_vecCols[iCol]; }
		t_col& GetColumn(std::size_t iCol) { return m_vecCols[iCol]; }
		std::size_t GetColumnCount() const { return m_vecCols.size(); }
		std::size_t GetRowCount() const { return m_vecCols[0].size(); }

		const std::vector<t_str>& GetRawHeader() const { return m_vecRawHdr; }
		const t_map& GetHeader() const { return m_mapHdr; }

	public:
		DatFile() = default;
		virtual ~DatFile() = default;

		DatFile(const t_str& strFile)
		{
			Load(strFile);
		}
};


// ----------------------------------------------------------------------------


// interface for instrument-specific data files
template<class _t_real = double>
class FileInstrBase
{
	public:
		using t_real = _t_real;

		typedef std::unordered_map<std::string, std::string> t_mapParams;
		typedef std::vector<std::string> t_vecColNames;
		typedef std::vector<t_real> t_vecVals;
		typedef std::vector<t_vecVals> t_vecDat;

	protected:
		void RenameDuplicateCols();

		std::array<t_real, 5> GetScanHKLKiKf(const char* pcH, const char* pcK,
			const char* pcL, const char* pcE, std::size_t i) const;

	public:
		FileInstrBase() = default;
		virtual ~FileInstrBase() = default;

		virtual bool Load(const char* pcFile) = 0;

		virtual std::array<t_real, 3> GetSampleLattice() const = 0;
		virtual std::array<t_real, 3> GetSampleAngles() const = 0;
		virtual std::array<t_real, 2> GetMonoAnaD() const = 0;

		virtual std::array<bool, 3> GetScatterSenses() const = 0;
		virtual std::array<t_real, 3> GetScatterPlane0() const = 0;
		virtual std::array<t_real, 3> GetScatterPlane1() const = 0;

		virtual t_real GetKFix() const = 0;
		virtual bool IsKiFixed() const = 0;

		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const = 0;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) = 0;

		virtual std::size_t GetScanCount() const = 0;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const = 0;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat);
		virtual void SmoothData(const std::string& strCol, t_real dEps, bool bIterate=1);

		virtual std::string GetTitle() const = 0;
		virtual std::string GetUser() const = 0;
		virtual std::string GetLocalContact() const = 0;
		virtual std::string GetScanNumber() const = 0;
		virtual std::string GetSampleName() const = 0;
		virtual std::string GetSpacegroup() const = 0;
		virtual std::string GetTimestamp() const = 0;

		virtual const t_vecDat& GetData() const = 0;
		virtual t_vecDat& GetData() = 0;
		virtual const t_vecColNames& GetColNames() const = 0;
		virtual const t_mapParams& GetAllParams() const = 0;

		virtual std::vector<std::string> GetScannedVars() const = 0;
		virtual std::string GetCountVar() const = 0;
		virtual std::string GetMonVar() const = 0;

		virtual std::string GetScanCommand() const = 0;

		// polarisation stuff
		virtual void ParsePolData();
		virtual std::size_t NumPolChannels() const;
		virtual const std::vector<std::array<t_real, 6>>& GetPolStates() const;
		virtual void SetPolNames(const char* pVec1, const char* pVec2,
			const char* pCur1, const char* pCur2);

	public:
		virtual bool MatchColumn(const std::string& strRegex,
			std::string& strColName, bool bSortByCounts=0, bool bFilterEmpty=1) const;

		static FileInstrBase<t_real>* LoadInstr(const char* pcFile);
};


// psi & ill files
template<class _t_real = double>
class FilePsi : public FileInstrBase<_t_real>
{
	public:
		using t_real = _t_real;
		using t_mapParams = typename FileInstrBase<t_real>::t_mapParams;
		using t_vecColNames = typename FileInstrBase<t_real>::t_vecColNames;
		using t_vecVals = typename FileInstrBase<t_real>::t_vecVals;
		using t_vecDat = typename FileInstrBase<t_real>::t_vecDat;

		// internal parameters in m_mapParams
		typedef std::map<std::string, t_real> t_mapIParams;

	protected:
		t_mapParams m_mapParams;
		t_mapIParams m_mapParameters, m_mapZeros, m_mapVariables, m_mapPosHkl, m_mapScanSteps;
		t_vecColNames m_vecColNames;
		t_vecDat m_vecData;

		// automatically look and parse for polarisation data
		bool m_bAutoParsePol = false;

		// incoming and outgoing polarisation states
		std::vector<std::array<t_real, 6>> m_vecPolStates;

		// instrument-specific device names
		std::string m_strPolVec1 {"p1"}, m_strPolVec2 {"p2"};
		std::string m_strPolCur1 {"i1"}, m_strPolCur2 {"i2"};


	protected:
		void ReadData(std::istream& istr);
		void GetInternalParams(const std::string& strAll, t_mapIParams& mapPara);

	public:
		FilePsi() = default;
		virtual ~FilePsi() = default;

		virtual bool Load(const char* pcFile) override;

		void PrintParams(std::ostream& ostr) const;
		const t_mapParams& GetParams() const { return m_mapParams; }

		const std::string& GetColName(std::size_t iCol) const { return m_vecColNames[iCol]; }
		std::size_t GetColCount() const { return m_vecColNames.size(); }

		const t_vecVals& GetCol(std::size_t iCol) const { return m_vecData[iCol]; }
		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const override;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) override;

	public:
		virtual std::array<t_real, 3> GetSampleLattice() const override;
		virtual std::array<t_real, 3> GetSampleAngles() const override;
		virtual std::array<t_real, 2> GetMonoAnaD() const override;

		virtual std::array<bool, 3> GetScatterSenses() const override;
		virtual std::array<t_real, 3> GetScatterPlane0() const override;
		virtual std::array<t_real, 3> GetScatterPlane1() const override;

		virtual t_real GetKFix() const override;
		virtual bool IsKiFixed() const override;

		std::array<t_real, 4> GetPosHKLE() const;	// zero pos.
		std::array<t_real, 4> GetDeltaHKLE() const;	// scan steps

		virtual std::size_t GetScanCount() const override;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const override;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat) override;

		virtual std::string GetTitle() const override;
		virtual std::string GetUser() const override;
		virtual std::string GetLocalContact() const override;
		virtual std::string GetScanNumber() const override;
		virtual std::string GetSampleName() const override;
		virtual std::string GetSpacegroup() const override;
		virtual std::string GetTimestamp() const override;

		virtual const t_vecDat& GetData() const override { return m_vecData; }
		virtual t_vecDat& GetData() override { return m_vecData; }
		virtual const t_vecColNames& GetColNames() const override { return m_vecColNames; }
		virtual const t_mapParams& GetAllParams() const override { return m_mapParams; }

		virtual std::vector<std::string> GetScannedVars() const override;
		virtual std::string GetCountVar() const override;
		virtual std::string GetMonVar() const override;

		virtual std::string GetScanCommand() const override;

		virtual std::size_t NumPolChannels() const override;

	public:
		void SetAutoParsePolData(bool b) { m_bAutoParsePol = b; }
		virtual void ParsePolData() override;

		virtual const std::vector<std::array<t_real, 6>>& GetPolStates() const override
		{
			return m_vecPolStates;
		}

		virtual void SetPolNames(const char* pVec1, const char* pVec2,
			const char* pCur1, const char* pCur2) override
		{
			m_strPolVec1 = pVec1; m_strPolVec2 = pVec2;
			m_strPolCur1 = pCur1; m_strPolCur2 = pCur2;
		}
};


// frm/nicos files
template<class _t_real = double>
class FileFrm : public FileInstrBase<_t_real>
{
	public:
		using t_real = _t_real;
		using t_mapParams = typename FileInstrBase<t_real>::t_mapParams;
		using t_vecColNames = typename FileInstrBase<t_real>::t_vecColNames;
		using t_vecVals = typename FileInstrBase<t_real>::t_vecVals;
		using t_vecDat = typename FileInstrBase<t_real>::t_vecDat;

	protected:
		t_mapParams m_mapParams;
		t_vecColNames m_vecQuantities, m_vecUnits;
		t_vecDat m_vecData;

	public:
		FileFrm() = default;
		virtual ~FileFrm() = default;

	protected:
		void ReadHeader(std::istream& istr);
		void ReadData(std::istream& istr);

	public:
		virtual bool Load(const char* pcFile) override;

		virtual std::array<t_real, 3> GetSampleLattice() const override;
		virtual std::array<t_real, 3> GetSampleAngles() const override;
		virtual std::array<t_real, 2> GetMonoAnaD() const override;

		virtual std::array<bool, 3> GetScatterSenses() const override;
		virtual std::array<t_real, 3> GetScatterPlane0() const override;
		virtual std::array<t_real, 3> GetScatterPlane1() const override;

		virtual t_real GetKFix() const override;
		virtual bool IsKiFixed() const override;

		virtual std::size_t GetScanCount() const override;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const override;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat) override;

		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const override;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) override;

		virtual std::string GetTitle() const override;
		virtual std::string GetUser() const override;
		virtual std::string GetLocalContact() const override;
		virtual std::string GetScanNumber() const override;
		virtual std::string GetSampleName() const override;
		virtual std::string GetSpacegroup() const override;
		virtual std::string GetTimestamp() const override;

		virtual const t_vecDat& GetData() const override { return m_vecData; }
		virtual t_vecDat& GetData() override { return m_vecData; }
		virtual const t_vecColNames& GetColNames() const override { return m_vecQuantities; }
		virtual const t_mapParams& GetAllParams() const override { return m_mapParams; }

		virtual std::vector<std::string> GetScannedVars() const override;
		virtual std::string GetCountVar() const override;
		virtual std::string GetMonVar() const override;

		virtual std::string GetScanCommand() const override;
};


// macs files
template<class _t_real = double>
class FileMacs : public FileInstrBase<_t_real>
{
	public:
		using t_real = _t_real;
		using t_mapParams = typename FileInstrBase<t_real>::t_mapParams;
		using t_vecColNames = typename FileInstrBase<t_real>::t_vecColNames;
		using t_vecVals = typename FileInstrBase<t_real>::t_vecVals;
		using t_vecDat = typename FileInstrBase<t_real>::t_vecDat;

	protected:
		t_mapParams m_mapParams;
		t_vecColNames m_vecQuantities;
		t_vecDat m_vecData;

	public:
		FileMacs() = default;
		virtual ~FileMacs() = default;

	protected:
		void ReadHeader(std::istream& istr);
		void ReadData(std::istream& istr);

	public:
		virtual bool Load(const char* pcFile) override;

		virtual std::array<t_real, 3> GetSampleLattice() const override;
		virtual std::array<t_real, 3> GetSampleAngles() const override;
		virtual std::array<t_real, 2> GetMonoAnaD() const override;

		virtual std::array<bool, 3> GetScatterSenses() const override;
		virtual std::array<t_real, 3> GetScatterPlane0() const override;
		virtual std::array<t_real, 3> GetScatterPlane1() const override;

		virtual t_real GetKFix() const override;
		virtual bool IsKiFixed() const override;

		virtual std::size_t GetScanCount() const override;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const override;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat) override;

		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const override;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) override;

		virtual std::string GetTitle() const override;
		virtual std::string GetUser() const override;
		virtual std::string GetLocalContact() const override;
		virtual std::string GetScanNumber() const override;
		virtual std::string GetSampleName() const override;
		virtual std::string GetSpacegroup() const override;
		virtual std::string GetTimestamp() const override;

		virtual const t_vecDat& GetData() const override { return m_vecData; }
		virtual t_vecDat& GetData() override { return m_vecData; }
		virtual const t_vecColNames& GetColNames() const override { return m_vecQuantities; }
		virtual const t_mapParams& GetAllParams() const override { return m_mapParams; }

		virtual std::vector<std::string> GetScannedVars() const override;
		virtual std::string GetCountVar() const override;
		virtual std::string GetMonVar() const override;

		virtual std::string GetScanCommand() const override;
};


// trisp files
template<class _t_real = double>
class FileTrisp : public FileInstrBase<_t_real>
{
	public:
		using t_real = _t_real;
		using t_mapParams = typename FileInstrBase<t_real>::t_mapParams;
		using t_vecColNames = typename FileInstrBase<t_real>::t_vecColNames;
		using t_vecVals = typename FileInstrBase<t_real>::t_vecVals;
		using t_vecDat = typename FileInstrBase<t_real>::t_vecDat;

	protected:
		t_mapParams m_mapParams;
		t_vecColNames m_vecQuantities;
		t_vecDat m_vecData;

	public:
		FileTrisp() = default;
		virtual ~FileTrisp() = default;

	protected:
		void ReadHeader(std::istream& istr);
		void ReadData(std::istream& istr);

	public:
		virtual bool Load(const char* pcFile) override;

		virtual std::array<t_real, 3> GetSampleLattice() const override;
		virtual std::array<t_real, 3> GetSampleAngles() const override;
		virtual std::array<t_real, 2> GetMonoAnaD() const override;

		virtual std::array<bool, 3> GetScatterSenses() const override;
		virtual std::array<t_real, 3> GetScatterPlane0() const override;
		virtual std::array<t_real, 3> GetScatterPlane1() const override;

		virtual t_real GetKFix() const override;
		virtual bool IsKiFixed() const override;

		virtual std::size_t GetScanCount() const override;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const override;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat) override;

		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const override;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) override;

		virtual std::string GetTitle() const override;
		virtual std::string GetUser() const override;
		virtual std::string GetLocalContact() const override;
		virtual std::string GetScanNumber() const override;
		virtual std::string GetSampleName() const override;
		virtual std::string GetSpacegroup() const override;
		virtual std::string GetTimestamp() const override;

		virtual const t_vecDat& GetData() const override { return m_vecData; }
		virtual t_vecDat& GetData() override { return m_vecData; }
		virtual const t_vecColNames& GetColNames() const override { return m_vecQuantities; }
		virtual const t_mapParams& GetAllParams() const override { return m_mapParams; }

		virtual std::vector<std::string> GetScannedVars() const override;
		virtual std::string GetCountVar() const override;
		virtual std::string GetMonVar() const override;

		virtual std::string GetScanCommand() const override;
};


// raw data files
template<class _t_real = double>
class FileRaw : public FileInstrBase<_t_real>
{
	public:
		using t_real = _t_real;
		using t_mapParams = typename FileInstrBase<t_real>::t_mapParams;
		using t_vecColNames = typename FileInstrBase<t_real>::t_vecColNames;
		using t_vecVals = typename FileInstrBase<t_real>::t_vecVals;
		using t_vecDat = typename FileInstrBase<t_real>::t_vecDat;

	protected:
		DatFile<t_real, char> m_dat;
		t_vecColNames m_vecCols;

	public:
		FileRaw() = default;
		virtual ~FileRaw() = default;

	public:
		virtual bool Load(const char* pcFile) override;

		virtual std::array<t_real, 3> GetSampleLattice() const override;
		virtual std::array<t_real, 3> GetSampleAngles() const override;
		virtual std::array<t_real, 2> GetMonoAnaD() const override;

		virtual std::array<bool, 3> GetScatterSenses() const override;
		virtual std::array<t_real, 3> GetScatterPlane0() const override;
		virtual std::array<t_real, 3> GetScatterPlane1() const override;

		virtual t_real GetKFix() const override;
		virtual bool IsKiFixed() const override;

		virtual std::size_t GetScanCount() const override;
		virtual std::array<t_real, 5> GetScanHKLKiKf(std::size_t i) const override;
		virtual bool MergeWith(const FileInstrBase<t_real>* pDat) override;

		virtual const t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) const override;
		virtual t_vecVals& GetCol(const std::string& strName, std::size_t *pIdx=0) override;

		virtual std::string GetTitle() const override;
		virtual std::string GetUser() const override;
		virtual std::string GetLocalContact() const override;
		virtual std::string GetScanNumber() const override;
		virtual std::string GetSampleName() const override;
		virtual std::string GetSpacegroup() const override;
		virtual std::string GetTimestamp() const override;

		virtual const t_vecDat& GetData() const override;
		virtual t_vecDat& GetData() override;
		virtual const t_vecColNames& GetColNames() const override;
		virtual const t_mapParams& GetAllParams() const override;

		virtual std::vector<std::string> GetScannedVars() const override;
		virtual std::string GetCountVar() const override;
		virtual std::string GetMonVar() const override;

		virtual std::string GetScanCommand() const override;
};



// ----------------------------------------------------------------------------
// implementation


template<class t_real>
void FileInstrBase<t_real>::RenameDuplicateCols()
{
	using t_mapCols = std::unordered_map<std::string, std::size_t>;
	t_mapCols mapCols;

	t_vecColNames& vecCols = const_cast<t_vecColNames&>(this->GetColNames());
	for(std::string& strCol : vecCols)
	{
		t_mapCols::iterator iter = mapCols.find(strCol);
		if(iter == mapCols.end())
		{
			mapCols.insert(std::make_pair(strCol, 0));
		}
		else
		{
			log_warn("Column \"", strCol, "\" is duplicate, renaming it.");

			++iter->second;
			strCol += "_" + var_to_str(iter->second);
		}
	}
}


// automatically choose correct instrument
template<class t_real>
FileInstrBase<t_real>* FileInstrBase<t_real>::LoadInstr(const char* pcFile)
{
	FileInstrBase<t_real>* pDat = nullptr;

	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
		return nullptr;

	std::string strLine, strLine2, strLine3;
	std::getline(ifstr, strLine);
	std::getline(ifstr, strLine2);
	std::getline(ifstr, strLine3);


	trim(strLine);
	trim(strLine2);
	trim(strLine3);
	strLine = str_to_lower(strLine);
	strLine2 = str_to_lower(strLine2);
	strLine3 = str_to_lower(strLine3);

	if(strLine == "")
		return nullptr;

	const std::string strNicos("nicos data file");
	const std::string strMacs("ice");
	const std::string strPsi("tas data");
	const std::string strPsiOld("instr:");

	if(strLine.find(strNicos) != std::string::npos)
	{ // frm file
		//log_debug(pcFile, " is an frm file.");
		pDat = new FileFrm<t_real>();
	}
	else if(strLine.find('#') != std::string::npos &&
		strLine.find(strMacs) != std::string::npos &&
		strLine2.find('#') != std::string::npos)
	{ // macs file
		//log_debug(pcFile, " is a macs file.");
		pDat = new FileMacs<t_real>();
	}
	else if(strLine2.find("scan start") != std::string::npos)
	{ // trisp file
		//log_debug(pcFile, " is a trisp file.");
		pDat = new FileTrisp<t_real>();
	}
	else if(strLine.find('#') == std::string::npos &&
		strLine2.find('#') == std::string::npos &&
		(strLine3.find(strPsi) != std::string::npos ||
		strLine.find(strPsiOld) != std::string::npos))
	{ // psi or ill file
		//log_debug(pcFile, " is an ill or psi file.");
		pDat = new FilePsi<t_real>();
	}
	else
	{ // raw file
		log_warn("\"", pcFile, "\" is of unknown type, falling back to raw loader.");
		pDat = new FileRaw<t_real>();
	}

	if(pDat && !pDat->Load(pcFile))
	{
		delete pDat;
		return nullptr;
	}

	return pDat;
}


template<class t_real>
std::array<t_real, 5> FileInstrBase<t_real>::GetScanHKLKiKf(const char* pcH, const char* pcK,
	const char* pcL, const char* pcE, std::size_t i) const
{
	const t_vecVals& vecH = GetCol(pcH);
	const t_vecVals& vecK = GetCol(pcK);
	const t_vecVals& vecL = GetCol(pcL);
	const t_vecVals& vecE = GetCol(pcE);

	std::size_t minSize = min4(vecH.size(), vecK.size(), vecL.size(), vecE.size());
	if(i>=minSize)
	{
		log_err("Scan position ", i, " out of bounds. Size: ", minSize, ".");
		return std::array<t_real,5>{{0.,0.,0.,0.}};
	}

	t_real h = vecH[i];
	t_real k = vecK[i];
	t_real l = vecL[i];
	t_real E = vecE[i];

	bool bKiFix = IsKiFixed();
	t_real kfix = GetKFix();
	t_real kother = get_other_k<units::si::system, t_real>
		(E*meV<t_real>, kfix/angstrom<t_real>, bKiFix) * angstrom<t_real>;

	return std::array<t_real,5>{{h,k,l, bKiFix?kfix:kother, bKiFix?kother:kfix}};
}


template<class t_real>
bool FileInstrBase<t_real>::MatchColumn(const std::string& strRegex,
	std::string& strColName, bool bSortByCounts, bool bFilterEmpty) const
{
	const FileInstrBase<t_real>::t_vecColNames& vecColNames = GetColNames();
	rex::regex rx(strRegex, rex::regex::ECMAScript | rex::regex_constants::icase);

	using t_pairCol = std::pair<std::string, t_real>;
	std::vector<t_pairCol> vecMatchedCols;

	for(const std::string& strCurColName : vecColNames)
	{
		rex::smatch m;
		if(rex::regex_match(strCurColName, m, rx))
		{
			const typename FileInstrBase<t_real>::t_vecVals& vecVals = GetCol(strCurColName);
			/*if(std::find_if(vecVals.begin(), vecVals.end(),
				[](const typename FileInstrBase<t_real>::t_vecVals::value_type& val) ->bool
				{ return !float_equal<t_real>(val, 0.); }) != vecVals.end())
			{
				strColName = strCurColName;
				return true;
			}*/

			t_real dSum = std::accumulate(vecVals.begin(), vecVals.end(), 0.,
				[](t_real t1, t_real t2) -> t_real { return t1+t2; });
			if(!bFilterEmpty || !float_equal<t_real>(dSum, 0.))
				vecMatchedCols.push_back(t_pairCol{strCurColName, dSum});
		}
	}

	if(bSortByCounts)
	{
		std::sort(vecMatchedCols.begin(), vecMatchedCols.end(),
		[](const t_pairCol& pair1, const t_pairCol& pair2) -> bool
			{ return pair1.second > pair2.second; });
	}

	if(vecMatchedCols.size())
	{
		strColName = vecMatchedCols[0].first;
		return true;
	}
	return false;
}


template<class t_real>
bool FileInstrBase<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	if(this->GetColNames().size() != pDat->GetColNames().size())
	{
		log_err("Cannot merge: Mismatching number of columns.");
		return false;
	}

	for(const std::string& strCol : GetColNames())
	{
		t_vecVals& col1 = this->GetCol(strCol);
		const t_vecVals& col2 = pDat->GetCol(strCol);
		//std::cout << "merging \"" << strCol << "\", size 1: " << col1.size() << ", size 2: " << col2.size() << std::endl;

		if(col1.size() == 0 || col2.size() == 0)
		{
			log_err("Cannot merge: Column \"", strCol, "\" is empty.");
			return false;
		}

		col1.insert(col1.end(), col2.begin(), col2.end());
	}

	return true;
}


template<class t_real>
void FileInstrBase<t_real>::SmoothData(const std::string& strCol,
	t_real dEps, bool bIterate)
{
	std::size_t iIdxCol;
	this->GetCol(strCol, &iIdxCol);		// get column index
	if(iIdxCol == GetColNames().size())	// no such column?
	{
		log_err("No such data column: \"", strCol, "\".");
		return;
	}

	while(1)
	{
		t_vecDat& vecDatOld = this->GetData();
		const std::size_t iNumCols = vecDatOld.size();
		const std::size_t iNumRows = vecDatOld[0].size();
		t_vecDat vecDatNew(iNumCols);
		std::vector<bool> vecValidRows(iNumRows, 1);

		for(std::size_t iPt1=0; iPt1<iNumRows; ++iPt1)
		{
			if(!vecValidRows[iPt1]) continue;

			t_vecVals vecVals(iNumCols, 0);
			std::size_t iNumUnited = 0;
			for(std::size_t iPt2=iPt1; iPt2<iNumRows; ++iPt2)
			{
				if(!vecValidRows[iPt2]) continue;
				if(std::abs(vecDatOld[iIdxCol][iPt1]-vecDatOld[iIdxCol][iPt2]) <= dEps)
				{
					for(std::size_t iCol=0; iCol<iNumCols; ++iCol)
						vecVals[iCol] += vecDatOld[iCol][iPt2];
					++iNumUnited;
					vecValidRows[iPt2] = 0;
				}
			}

			for(std::size_t iCol=0; iCol<iNumCols; ++iCol)
			{
				vecVals[iCol] /= t_real(iNumUnited);
				vecDatNew[iCol].push_back(vecVals[iCol]);
			}
		}

		vecDatOld = vecDatNew;
		// if no iteration requested or no more change -> break
		if(!bIterate || iNumRows==vecDatNew[0].size())
			break;
	}
}


template<class t_real>
void FileInstrBase<t_real>::ParsePolData()
{}

template<class t_real>
void FileInstrBase<t_real>::SetPolNames(const char* pVec1, const char* pVec2,
	const char* pCur1, const char* pCur2)
{}

template<class t_real>
std::size_t FileInstrBase<t_real>::NumPolChannels() const
{ return 0; }

template<class t_real>
const std::vector<std::array<t_real, 6>>& FileInstrBase<t_real>::GetPolStates() const
{
	static const std::vector<std::array<t_real, 6>> vecNull;
	return vecNull;
}



// -----------------------------------------------------------------------------


template<class t_real>
void FilePsi<t_real>::ReadData(std::istream& istr)
{
	std::size_t iLine = 0;

	// header
	std::string strHdr;
	std::getline(istr, strHdr);
	trim(strHdr);
	++iLine;
	get_tokens<std::string, std::string, t_vecColNames>(strHdr, " \t", m_vecColNames);
	for(std::string& _str : m_vecColNames)
	{
		boost::replace_all<std::string, std::string, std::string>(_str, "\n", "");
		boost::replace_all<std::string, std::string, std::string>(_str, "\r", "");
	}

	m_vecData.resize(m_vecColNames.size());
	FileInstrBase<t_real>::RenameDuplicateCols();


	// data
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);
		++iLine;
		trim(strLine);

		if(strLine.length() == 0)
			continue;
		if(strLine[0] == '#')
			continue;

		std::vector<t_real> vecToks;
		get_tokens<t_real, std::string>(strLine, " \t", vecToks);

		if(vecToks.size() != m_vecColNames.size())
		{
			log_warn("Loader: Column size mismatch in data line ", iLine,
				": Expected ", m_vecColNames.size(), ", got ", vecToks.size(), ".");

			// add zeros
			while(m_vecColNames.size() > vecToks.size())
				vecToks.push_back(0.);
		}

		for(std::size_t iTok=0; iTok<vecToks.size(); ++iTok)
			m_vecData[iTok].push_back(vecToks[iTok]);
	}
}


template<class t_real>
void FilePsi<t_real>::GetInternalParams(const std::string& strAll, FilePsi<t_real>::t_mapIParams& mapPara)
{
	std::vector<std::string> vecToks;
	get_tokens<std::string, std::string>(strAll, ",\n", vecToks);
	//std::cout << strAll << std::endl;

	for(const std::string& strTok : vecToks)
	{
		std::pair<std::string, std::string> pair =
				split_first<std::string>(strTok, "=", 1);

		if(pair.first == "")
			continue;

		t_real dVal = str_to_var<t_real>(pair.second);
		mapPara.insert(typename t_mapIParams::value_type(pair.first, dVal));

		//std::cout << "Key: " << pair.first << ", Val: " << dVal << std::endl;
	}
}


template<class t_real>
void FilePsi<t_real>::ParsePolData()
{
	m_vecPolStates.clear();
	typename t_mapParams::const_iterator iter = m_mapParams.find("POLAN");
	if(iter == m_mapParams.end())
		return;

	std::vector<std::string> vecLines;
	get_tokens<std::string, std::string>(iter->second, ",", vecLines);

	// initial and final polarisation states
	t_real Pix = t_real(0), Piy = t_real(0), Piz = t_real(0);
	t_real Pfx = t_real(0), Pfy = t_real(0), Pfz = t_real(0);
	t_real Pi_sign = t_real(1);
	t_real Pf_sign = t_real(1);


	// check if a string describes a number
	auto is_num = [](const std::string& str) -> bool
	{
		for(typename std::string::value_type c : str)
		{
			if(c!='0' && c!='1' && c!='2' && c!='3' && c!='4'
				&& c!='5' && c!='6' && c!='7' && c!='8' && c!='9'
				&& c!='+'&& c!='-'&& c!='.')
				return false;
		}
		return true;
	};


	// iterate command lines
	for(std::string& strLine : vecLines)
	{
		trim(strLine);
		strLine = str_to_lower(strLine);

		std::vector<std::string> vecLine;
		get_tokens<std::string, std::string>(strLine, " \t", vecLine);

		if(vecLine.size() == 0)
			continue;

		if(vecLine[0] == "dr")	// polarisation vector or current driven
		{
			std::string strCurDev = "";

			std::size_t iCurComp = 0;
			for(std::size_t iDr=1; iDr<vecLine.size(); ++iDr)
			{
				const std::string& strWord = vecLine[iDr];

				if(is_num(strWord))	// value to drive to
				{
					t_real dNum = str_to_var<t_real>(strWord);

					if(strCurDev == m_strPolVec1)
					{	// incoming polarisation vector changed
						switch(iCurComp)
						{
							case 0: Pix = dNum; break;
							case 1: Piy = dNum; break;
							case 2: Piz = dNum; break;
						}
					}
					else if(strCurDev == m_strPolVec2)
					{	// outgoing polarisation vector changed
						switch(iCurComp)
						{
							case 0: Pfx = dNum; break;
							case 1: Pfy = dNum; break;
							case 2: Pfz = dNum; break;
						}
					}
					else if(strCurDev == m_strPolCur1)
					{	// sign of polarisation vector 1 changed
						if(iCurComp == 0)
						{
							if(dNum >= t_real(0))
								Pi_sign = t_real(1);
							else
								Pi_sign = t_real(-1);
						}
					}
					else if(strCurDev == m_strPolCur2)
					{	// sign of polarisation vector 2 changed
						if(iCurComp == 0)
						{
							if(dNum >= t_real(0))
								Pf_sign = t_real(1);
							else
								Pf_sign = t_real(-1);
						}
					}

					++iCurComp;
				}
				else	// (next) device to drive
				{
					strCurDev = strWord;
					iCurComp = 0;
				}
			}
		}
		else if(vecLine[0] == "co")	// count command issued -> save current spin states
		{
			m_vecPolStates.push_back(std::array<t_real,6>({{
				Pi_sign*Pix, Pi_sign*Piy, Pi_sign*Piz,
				Pf_sign*Pfx, Pf_sign*Pfy, Pf_sign*Pfz }}));

			//std::cout << Pi_sign*Pix << " " << Pi_sign*Piy << " " << Pi_sign*Piz << " -> "
			//	<< Pf_sign*Pfx << " " << Pf_sign*Pfy << " " << Pf_sign*Pfz << std::endl;
		}
	}


	// cleanup
	for(std::size_t iPol=0; iPol<m_vecPolStates.size(); ++iPol)
	{
		for(unsigned iComp=0; iComp<6; ++iComp)
			set_eps_0(m_vecPolStates[iPol][iComp]);
	}
}


template<class t_real>
bool FilePsi<t_real>::Load(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
		return false;

	while(!ifstr.eof())
	{
		std::string strLine;
		std::getline(ifstr, strLine);

		if(strLine.substr(0,4) == "RRRR")
			skip_after_line<char>(ifstr, "VVVV", true);

		std::pair<std::string, std::string> pairLine =
				split_first<std::string>(strLine, ":", 1);
		if(pairLine.first == "DATA_")
			ReadData(ifstr);
		else if(pairLine.first == "")
			continue;
		else
		{
			typename t_mapParams::iterator iter = m_mapParams.find(pairLine.first);

			if(iter == m_mapParams.end())
				m_mapParams.insert(pairLine);
			else
				iter->second += ", " + pairLine.second;
		}
	}

	typename t_mapParams::const_iterator iterParams = m_mapParams.find("PARAM"),
		iterZeros = m_mapParams.find("ZEROS"),
		iterVars = m_mapParams.find("VARIA"),
		iterPos = m_mapParams.find("POSQE"),
		iterSteps = m_mapParams.find("STEPS");

	if(iterParams!=m_mapParams.end()) GetInternalParams(iterParams->second, m_mapParameters);
	if(iterZeros!=m_mapParams.end()) GetInternalParams(iterZeros->second, m_mapZeros);
	if(iterVars!=m_mapParams.end()) GetInternalParams(iterVars->second, m_mapVariables);
	if(iterPos!=m_mapParams.end()) GetInternalParams(iterPos->second, m_mapPosHkl);
	if(iterSteps!=m_mapParams.end()) GetInternalParams(iterSteps->second, m_mapScanSteps);

	if(m_bAutoParsePol)
		ParsePolData();
	return true;
}


template<class t_real>
const typename FileInstrBase<t_real>::t_vecVals& 
FilePsi<t_real>::GetCol(const std::string& strName, std::size_t *pIdx) const
{
	return const_cast<FilePsi*>(this)->GetCol(strName, pIdx);
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecVals& 
FilePsi<t_real>::GetCol(const std::string& strName, std::size_t *pIdx)
{
	static std::vector<t_real> vecNull;

	for(std::size_t i=0; i<m_vecColNames.size(); ++i)
	{
		if(str_to_lower(m_vecColNames[i]) == str_to_lower(strName))
		{
			if(pIdx) *pIdx = i;
			return m_vecData[i];
		}
	}

	if(pIdx) *pIdx = m_vecColNames.size();
	return vecNull;
}

template<class t_real>
void FilePsi<t_real>::PrintParams(std::ostream& ostr) const
{
	for(const typename t_mapParams::value_type& val : m_mapParams)
	{
		ostr << "Param: " << val.first
			<< ", Val: " << val.second << "\n";
	}
}


template<class t_real>
std::array<t_real,3> FilePsi<t_real>::GetSampleLattice() const
{
	typename t_mapIParams::const_iterator iterA = m_mapParameters.find("AS");
	typename t_mapIParams::const_iterator iterB = m_mapParameters.find("BS");
	typename t_mapIParams::const_iterator iterC = m_mapParameters.find("CS");

	t_real a = (iterA!=m_mapParameters.end() ? iterA->second : 0.);
	t_real b = (iterB!=m_mapParameters.end() ? iterB->second : 0.);
	t_real c = (iterC!=m_mapParameters.end() ? iterC->second : 0.);

	return std::array<t_real,3>{{a,b,c}};
}

template<class t_real>
std::array<t_real,3> FilePsi<t_real>::GetSampleAngles() const
{
	typename t_mapIParams::const_iterator iterA = m_mapParameters.find("AA");
	typename t_mapIParams::const_iterator iterB = m_mapParameters.find("BB");
	typename t_mapIParams::const_iterator iterC = m_mapParameters.find("CC");

	t_real alpha = (iterA!=m_mapParameters.end() ? d2r(iterA->second) : pi<t_real>/2.);
	t_real beta = (iterB!=m_mapParameters.end() ? d2r(iterB->second) : pi<t_real>/2.);
	t_real gamma = (iterC!=m_mapParameters.end() ? d2r(iterC->second) : pi<t_real>/2.);

	return std::array<t_real,3>{{alpha, beta, gamma}};
}

template<class t_real>
std::array<t_real,2> FilePsi<t_real>::GetMonoAnaD() const
{
	typename t_mapIParams::const_iterator iterM = m_mapParameters.find("DM");
	typename t_mapIParams::const_iterator iterA = m_mapParameters.find("DA");

	t_real m = (iterM!=m_mapParameters.end() ? iterM->second : 3.355);
	t_real a = (iterA!=m_mapParameters.end() ? iterA->second : 3.355);

	return std::array<t_real,2>{{m, a}};
}

template<class t_real>
std::array<bool, 3> FilePsi<t_real>::GetScatterSenses() const
{
	typename t_mapIParams::const_iterator iterM = m_mapParameters.find("SM");
	typename t_mapIParams::const_iterator iterS = m_mapParameters.find("SS");
	typename t_mapIParams::const_iterator iterA = m_mapParameters.find("SA");

	bool m = (iterM!=m_mapParameters.end() ? (iterM->second>0) : 0);
	bool s = (iterS!=m_mapParameters.end() ? (iterS->second>0) : 1);
	bool a = (iterA!=m_mapParameters.end() ? (iterA->second>0) : 0);

	return std::array<bool,3>{{m, s, a}};
}

template<class t_real>
std::array<t_real, 3> FilePsi<t_real>::GetScatterPlane0() const
{
	typename t_mapIParams::const_iterator iterX = m_mapParameters.find("AX");
	typename t_mapIParams::const_iterator iterY = m_mapParameters.find("AY");
	typename t_mapIParams::const_iterator iterZ = m_mapParameters.find("AZ");

	t_real x = (iterX!=m_mapParameters.end() ? iterX->second : 1.);
	t_real y = (iterY!=m_mapParameters.end() ? iterY->second : 0.);
	t_real z = (iterZ!=m_mapParameters.end() ? iterZ->second : 0.);

	return std::array<t_real,3>{{x,y,z}};
}

template<class t_real>
std::array<t_real, 3> FilePsi<t_real>::GetScatterPlane1() const
{
	typename t_mapIParams::const_iterator iterX = m_mapParameters.find("BX");
	typename t_mapIParams::const_iterator iterY = m_mapParameters.find("BY");
	typename t_mapIParams::const_iterator iterZ = m_mapParameters.find("BZ");

	t_real x = (iterX!=m_mapParameters.end() ? iterX->second : 0.);
	t_real y = (iterY!=m_mapParameters.end() ? iterY->second : 1.);
	t_real z = (iterZ!=m_mapParameters.end() ? iterZ->second : 0.);

	return std::array<t_real,3>{{x,y,z}};
}

template<class t_real>
t_real FilePsi<t_real>::GetKFix() const
{
	typename t_mapIParams::const_iterator iterK = m_mapParameters.find("KFIX");
	t_real k = (iterK!=m_mapParameters.end() ? iterK->second : 0.);

	return k;
}

template<class t_real>
std::array<t_real, 4> FilePsi<t_real>::GetPosHKLE() const
{
	typename t_mapIParams::const_iterator iterH = m_mapPosHkl.find("QH");
	typename t_mapIParams::const_iterator iterK = m_mapPosHkl.find("QK");
	typename t_mapIParams::const_iterator iterL = m_mapPosHkl.find("QL");
	typename t_mapIParams::const_iterator iterE = m_mapPosHkl.find("EN");

	t_real h = (iterH!=m_mapPosHkl.end() ? iterH->second : 0.);
	t_real k = (iterK!=m_mapPosHkl.end() ? iterK->second : 0.);
	t_real l = (iterL!=m_mapPosHkl.end() ? iterL->second : 0.);
	t_real E = (iterE!=m_mapPosHkl.end() ? iterE->second : 0.);

	return std::array<t_real,4>{{h,k,l,E}};
}

template<class t_real>
std::array<t_real, 4> FilePsi<t_real>::GetDeltaHKLE() const
{
	typename t_mapIParams::const_iterator iterH = m_mapScanSteps.find("DQH");
	if(iterH==m_mapScanSteps.end()) iterH = m_mapScanSteps.find("QH");

	typename t_mapIParams::const_iterator iterK = m_mapScanSteps.find("DQK");
	if(iterK==m_mapScanSteps.end()) iterK = m_mapScanSteps.find("QK");

	typename t_mapIParams::const_iterator iterL = m_mapScanSteps.find("DQL");
	if(iterL==m_mapScanSteps.end()) iterL = m_mapScanSteps.find("QL");

	typename t_mapIParams::const_iterator iterE = m_mapScanSteps.find("DEN");
	if(iterE==m_mapScanSteps.end()) iterE = m_mapScanSteps.find("EN");


        t_real h = (iterH!=m_mapScanSteps.end() ? iterH->second : 0.);
        t_real k = (iterK!=m_mapScanSteps.end() ? iterK->second : 0.);
        t_real l = (iterL!=m_mapScanSteps.end() ? iterL->second : 0.);
        t_real E = (iterE!=m_mapScanSteps.end() ? iterE->second : 0.);

        return std::array<t_real,4>{{h,k,l,E}};
}

template<class t_real>
bool FilePsi<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	if(!FileInstrBase<t_real>::MergeWith(pDat))
		return false;

	std::string strNr = pDat->GetScanNumber();
	if(strNr.length() != 0)
	{
		// include merged scan number
		typename t_mapParams::iterator iter = m_mapParams.find("FILE_");
		if(iter != m_mapParams.end())
			iter->second += std::string(" + ") + strNr;
	}

	return true;
}

// TODO
template<class t_real>
bool FilePsi<t_real>::IsKiFixed() const
{
	return 0;
}

template<class t_real>
std::size_t FilePsi<t_real>::GetScanCount() const
{
	if(m_vecData.size() < 1)
		return 0;
	return m_vecData[0].size();
}

template<class t_real>
std::array<t_real, 5> FilePsi<t_real>::GetScanHKLKiKf(std::size_t i) const
{
	return FileInstrBase<t_real>::GetScanHKLKiKf("QH", "QK", "QL", "EN", i);
}


template<class t_real>
std::string FilePsi<t_real>::GetTitle() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("TITLE");
	if(iter != m_mapParams.end())
		strTitle = iter->second;
	return strTitle;
}

template<class t_real>
std::string FilePsi<t_real>::GetUser() const
{
	std::string strUser;
	typename t_mapParams::const_iterator iter = m_mapParams.find("USER_");
	if(iter != m_mapParams.end())
		strUser = iter->second;
	return strUser;
}

template<class t_real>
std::string FilePsi<t_real>::GetLocalContact() const
{
	std::string strUser;
	typename t_mapParams::const_iterator iter = m_mapParams.find("LOCAL");
	if(iter != m_mapParams.end())
		strUser = iter->second;
	return strUser;
}

template<class t_real>
std::string FilePsi<t_real>::GetScanNumber() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("FILE_");
	if(iter != m_mapParams.end())
		strTitle = iter->second;
	return strTitle;
}

template<class t_real>
std::string FilePsi<t_real>::GetSampleName() const
{
	return "";
}

template<class t_real>
std::string FilePsi<t_real>::GetSpacegroup() const
{
	return "";
}


template<class t_real>
std::vector<std::string> FilePsi<t_real>::GetScannedVars() const
{
	std::vector<std::string> vecVars;

	// steps parameter
	for(const typename t_mapIParams::value_type& pair : m_mapScanSteps)
	{
		//std::cout << pair.first << ", " << pair.second << std::endl;
		if(!float_equal<t_real>(pair.second, 0.) && pair.first.length())
		{
			if(std::tolower(pair.first[0]) == 'd')
				vecVars.push_back(pair.first.substr(1));
			else
				vecVars.push_back(pair.first);
		}
	}


	// nothing found yet -> try scan command instead
	if(!vecVars.size())
	{
		typename t_mapParams::const_iterator iter = m_mapParams.find("COMND");
		if(iter != m_mapParams.end())
		{
			std::vector<std::string> vecToks;
			//std::cout << iter->second << std::endl;
			get_tokens<std::string, std::string>(iter->second, " \t", vecToks);
			for(std::string& strTok : vecToks)
				trim(strTok);

			std::transform(vecToks.begin(), vecToks.end(), vecToks.begin(), str_to_lower<std::string>);
			typename std::vector<std::string>::iterator iterTok
				= std::find(vecToks.begin(), vecToks.end(), "dqh");

			if(iterTok != vecToks.end())
			{
				t_real dh = str_to_var<t_real>(*(++iterTok));
				t_real dk = str_to_var<t_real>(*(++iterTok));
				t_real dl = str_to_var<t_real>(*(++iterTok));
				t_real dE = str_to_var<t_real>(*(++iterTok));

				if(!float_equal<t_real>(dh, 0.)) vecVars.push_back("QH");
				if(!float_equal<t_real>(dk, 0.)) vecVars.push_back("QK");
				if(!float_equal<t_real>(dl, 0.)) vecVars.push_back("QL");
				if(!float_equal<t_real>(dE, 0.)) vecVars.push_back("EN");
			}


			// still nothing found, try regex
			if(!vecVars.size())
			{
				const std::string strRegex = R"REX((sc|scan)[ \t]+([a-z0-9]+)[ \t]+[0-9\.-]+[ \t]+[d|D]([a-z0-9]+).*)REX";
				rex::regex rx(strRegex, rex::regex::ECMAScript|rex::regex_constants::icase);
				rex::smatch m;
				if(rex::regex_search(iter->second, m, rx) && m.size()>3)
				{
					const std::string& strSteps = m[3];
					vecVars.push_back(str_to_upper(strSteps));
				}
			}
		}
	}

	if(!vecVars.size())
	{
		log_warn("Could not determine scan variable.");
		if(m_vecColNames.size() >= 1)
		{
			log_warn("Using first column: \"", m_vecColNames[0], "\".");
			vecVars.push_back(m_vecColNames[0]);
		}
	}

	//for(std::string& strVar : vecVars)
	//	log_info("Scan var: ", strVar);
	//log_info("--------");
	return vecVars;
}


template<class t_real>
std::string FilePsi<t_real>::GetCountVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(cnts)REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FilePsi<t_real>::GetMonVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(m[0-9])REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FilePsi<t_real>::GetScanCommand() const
{
	std::string strCmd;
	typename t_mapParams::const_iterator iter = m_mapParams.find("COMND");
	if(iter != m_mapParams.end())
		strCmd = iter->second;
	return strCmd;
}

template<class t_real>
std::string FilePsi<t_real>::GetTimestamp() const
{
	std::string strDate;
	typename t_mapParams::const_iterator iter = m_mapParams.find("DATE_");
	if(iter != m_mapParams.end())
		strDate = iter->second;
	return strDate;
}


template<class t_real>
std::size_t FilePsi<t_real>::NumPolChannels() const
{
	return m_vecPolStates.size();
}


// -----------------------------------------------------------------------------



template<class t_real>
void FileFrm<t_real>::ReadHeader(std::istream& istr)
{
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);
		//std::cout << strLine << std::endl;
		trim(strLine);
		if(strLine.length()==0 || strLine[0]!='#')
			continue;
		if(strLine.length()>=3 && strLine[0]=='#' && strLine[1]=='#' && strLine[2]=='#')
		{
			std::string strCreatedAt("created at");
			std::size_t iPosCreated = strLine.find(strCreatedAt);
			if(iPosCreated != std::string::npos)
			{
				iPosCreated += strCreatedAt.length();
				std::string strDate = strLine.substr(iPosCreated);
				trim(strDate);

				m_mapParams["file_timestamp"] = strDate;
			}

			continue;
		}

		strLine = strLine.substr(1);
		//std::cout << strLine << std::endl;

		std::pair<std::string, std::string> pairLine =
				split_first<std::string>(strLine, ":", 1);
		if(pairLine.first == "")
			continue;
		else
		{
			//std::cout << "Key: " << pairLine.first << ", Val: " << pairLine.second << std::endl;
			typename t_mapParams::iterator iter = m_mapParams.find(pairLine.first);

			if(iter == m_mapParams.end())
				m_mapParams.insert(pairLine);
			else
				iter->second += ", " + pairLine.second;
		}
	}
}

template<class t_real>
void FileFrm<t_real>::ReadData(std::istream& istr)
{
	skip_after_line<char>(istr, "### scan data", true, false);

	// column headers
	skip_after_char<char>(istr, '#');
	std::string strLineQuantities;
	std::getline(istr, strLineQuantities);
	get_tokens<std::string, std::string, t_vecColNames>
		(strLineQuantities, " \t", m_vecQuantities);
	for(std::string& _str : m_vecQuantities)
	{
		boost::replace_all<std::string, std::string, std::string>(_str, "\n", "");
		boost::replace_all<std::string, std::string, std::string>(_str, "\r", "");
	}

	skip_after_char<char>(istr, '#');
	std::string strLineUnits;
	std::getline(istr, strLineUnits);
	get_tokens<std::string, std::string, t_vecColNames>
		(strLineQuantities, " \t", m_vecUnits);


	m_vecData.resize(m_vecQuantities.size());

	// data
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);
		trim(strLine);
		if(strLine.length()==0 || strLine[0]=='#')
			continue;

		std::vector<t_real> vecToks;
		get_tokens<t_real, std::string>(strLine, " \t", vecToks);

		if(vecToks.size() != m_vecQuantities.size())
		{
			log_warn("Loader: Line size mismatch.");

			// add zeros
			while(m_vecQuantities.size() > vecToks.size())
				vecToks.push_back(0.);
		}

		for(std::size_t iTok=0; iTok<vecToks.size(); ++iTok)
			m_vecData[iTok].push_back(vecToks[iTok]);
	}

	FileInstrBase<t_real>::RenameDuplicateCols();
}


template<class t_real>
bool FileFrm<t_real>::Load(const char* pcFile)
{
	for(int iStep : {0,1})
	{
		std::ifstream ifstr(pcFile);
		if(!ifstr.is_open())
			return false;

		//std::streampos posIstr = ifstr.tellg();
		//ReadHeader(ifstr);
		//ifstr.seekg(posIstr, std::ios_base::beg);
		//ifstr.clear();
		//ReadData(ifstr);

		if(iStep==0)
			ReadHeader(ifstr);
		else if(iStep==1)
			ReadData(ifstr);
	}

	return true;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecVals& 
FileFrm<t_real>::GetCol(const std::string& strName, std::size_t *pIdx) const
{
	return const_cast<FileFrm*>(this)->GetCol(strName, pIdx);
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecVals& 
FileFrm<t_real>::GetCol(const std::string& strName, std::size_t *pIdx)
{
	static std::vector<t_real> vecNull;

	for(std::size_t i=0; i<m_vecQuantities.size(); ++i)
	{
		if(m_vecQuantities[i] == strName)
		{
			if(pIdx) *pIdx = i;
			return m_vecData[i];
		}
	}

	if(pIdx) *pIdx = m_vecQuantities.size();
	return vecNull;
}


template<class t_real>
std::array<t_real, 3> FileFrm<t_real>::GetSampleLattice() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_lattice");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vec = get_py_array<std::string, std::vector<t_real>>(iter->second);
	if(vec.size() != 3)
	{
		log_err("Invalid lattice array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{vec[0],vec[1],vec[2]}};
}

template<class t_real>
std::array<t_real, 3> FileFrm<t_real>::GetSampleAngles() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_angles");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vec = get_py_array<std::string, std::vector<t_real>>(iter->second);
	if(vec.size() != 3)
	{
		log_err("Invalid angle array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{d2r(vec[0]), d2r(vec[1]), d2r(vec[2])}};
}

template<class t_real>
std::array<t_real, 2> FileFrm<t_real>::GetMonoAnaD() const
{
	typename t_mapParams::const_iterator iterM = m_mapParams.find("mono_dvalue");
	typename t_mapParams::const_iterator iterA = m_mapParams.find("ana_dvalue");

	t_real m = (iterM!=m_mapParams.end() ? str_to_var<t_real>(iterM->second) : 3.355);
	t_real a = (iterA!=m_mapParams.end() ? str_to_var<t_real>(iterA->second) : 3.355);

	return std::array<t_real,2>{{m, a}};
}

template<class t_real>
std::array<bool, 3> FileFrm<t_real>::GetScatterSenses() const
{
	std::vector<int> vec;

	typename t_mapParams::const_iterator iter;
	for(iter=m_mapParams.begin(); iter!=m_mapParams.end(); ++iter)
	{
		if(iter->first.find("scatteringsense") != std::string::npos)
		{
			vec = get_py_array<std::string, std::vector<int>>(iter->second);
			break;
		}
	}

	if(vec.size() != 3)
	{
		vec.resize(3);
		vec[0] = 0; vec[1] = 1; vec[2] = 0;
	}

	return std::array<bool,3>{{vec[0]>0, vec[1]>0, vec[2]>0}};
}

template<class t_real>
std::array<t_real, 3> FileFrm<t_real>::GetScatterPlane0() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_orient1");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vec = get_py_array<std::string, std::vector<t_real>>(iter->second);
	if(vec.size() != 3)
	{
		log_err("Invalid sample peak 1 array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}
	return std::array<t_real,3>{{vec[0],vec[1],vec[2]}};
}

template<class t_real>
std::array<t_real, 3> FileFrm<t_real>::GetScatterPlane1() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_orient2");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vec = get_py_array<std::string, std::vector<t_real>>(iter->second);
	if(vec.size() != 3)
	{
		log_err("Invalid sample peak 2 array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}
	return std::array<t_real,3>{{-vec[0],-vec[1],-vec[2]}};	// LH -> RH
}

template<class t_real>
t_real FileFrm<t_real>::GetKFix() const
{
	std::string strKey = (IsKiFixed() ? "ki_value" : "kf_value");

	typename t_mapParams::const_iterator iter = m_mapParams.find(strKey);
	return (iter!=m_mapParams.end() ? str_to_var<t_real>(iter->second) : 0.);
}

template<class t_real>
bool FileFrm<t_real>::IsKiFixed() const
{
	std::string strScanMode = "ckf";

	typename t_mapParams::const_iterator iter;
	for(iter=m_mapParams.begin(); iter!=m_mapParams.end(); ++iter)
	{
		if(iter->first.find("scanmode") != std::string::npos)
		{
			strScanMode = str_to_lower(iter->second);
			trim(strScanMode);
			break;
		}
	}

	if(strScanMode == "cki")
		return 1;
	return 0;
}

template<class t_real>
std::size_t FileFrm<t_real>::GetScanCount() const
{
	if(m_vecData.size() < 1)
		return 0;
	return m_vecData[0].size();
}

template<class t_real>
std::array<t_real, 5> FileFrm<t_real>::GetScanHKLKiKf(std::size_t i) const
{
	return FileInstrBase<t_real>::GetScanHKLKiKf("h", "k", "l", "E", i);
}

template<class t_real>
bool FileFrm<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	if(!FileInstrBase<t_real>::MergeWith(pDat))
		return false;

	std::string strNr = pDat->GetScanNumber();
	if(strNr.length() != 0)
	{
		// include merged scan number
		typename t_mapParams::iterator iter = m_mapParams.find("number");
		if(iter != m_mapParams.end())
			iter->second += std::string(" + ") + strNr;
	}

	return true;
}


template<class t_real>
std::string FileFrm<t_real>::GetTitle() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Exp_title");
	if(iter != m_mapParams.end())
		strTitle = iter->second;
	return strTitle;
}

template<class t_real>
std::string FileFrm<t_real>::GetUser() const
{
	std::string strUser;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Exp_users");
	if(iter != m_mapParams.end())
		strUser = iter->second;
	return strUser;
}

template<class t_real>
std::string FileFrm<t_real>::GetLocalContact() const
{
	std::string strUser;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Exp_localcontact");
	if(iter != m_mapParams.end())
		strUser = iter->second;
	return strUser;
}

template<class t_real>
std::string FileFrm<t_real>::GetScanNumber() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("number");
	if(iter != m_mapParams.end())
		strTitle = iter->second;
	return strTitle;
}

template<class t_real>
std::string FileFrm<t_real>::GetSampleName() const
{
	std::string strName;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_samplename");
	if(iter != m_mapParams.end())
		strName = iter->second;
	return strName;
}

template<class t_real>
std::string FileFrm<t_real>::GetSpacegroup() const
{
	std::string strSG;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Sample_spacegroup");
	if(iter != m_mapParams.end())
		strSG = iter->second;
	return strSG;
}


template<class t_real>
std::vector<std::string> FileFrm<t_real>::GetScannedVars() const
{
	std::vector<std::string> vecVars;

	// scan command
	typename t_mapParams::const_iterator iter = m_mapParams.find("info");
	if(iter != m_mapParams.end())
	{
		const std::string& strInfo = iter->second;

		// try qscan/qcscan
		const std::string strRegex = R"REX((qscan|qcscan)\((\[.*\])[, ]+(\[.*\]).*\))REX";
		rex::regex rx(strRegex, rex::regex::ECMAScript|rex::regex_constants::icase);
		rex::smatch m;
		if(rex::regex_search(strInfo, m, rx) && m.size()>3)
		{
			const std::string& strSteps = m[3];
			std::vector<t_real> vecSteps = get_py_array<std::string, std::vector<t_real>>(strSteps);

			if(vecSteps.size()>0 && !float_equal<t_real>(vecSteps[0], 0.))
				vecVars.push_back("h");
			if(vecSteps.size()>1 && !float_equal<t_real>(vecSteps[1], 0.))
				vecVars.push_back("k");
			if(vecSteps.size()>2 && !float_equal<t_real>(vecSteps[2], 0.))
				vecVars.push_back("l");
			if(vecSteps.size()>3 && !float_equal<t_real>(vecSteps[3], 0.))
				vecVars.push_back("E");
		}


		if(vecVars.size() == 0)
		{
			// try scan/cscan
			const std::string strRegexDevScan = R"REX((scan|cscan)\(([a-z0-9_\.]+)[, ]+.*\))REX";
			rex::regex rxDev(strRegexDevScan, rex::regex::ECMAScript|rex::regex_constants::icase);
			rex::smatch mDev;
			if(rex::regex_search(strInfo, mDev, rxDev) && mDev.size()>2)
			{
				std::string strDev = mDev[2];
				if(std::find(m_vecQuantities.begin(), m_vecQuantities.end(), strDev) != m_vecQuantities.end())
					vecVars.push_back(strDev);
			}
		}
	}

	if(!vecVars.size())
	{
		log_warn("Could not determine scan variable.");
		if(m_vecQuantities.size() >= 1)
		{
			log_warn("Using first column: \"", m_vecQuantities[0], "\".");
			vecVars.push_back(m_vecQuantities[0]);
		}
	}

	return vecVars;
}

template<class t_real>
std::string FileFrm<t_real>::GetCountVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX((det[a-z]*[0-9])|(ctr[0-9])|(counter[0-9])|([a-z0-9\.]*roi))REX", strRet, true))
		return strRet;
	return "";
}

template<class t_real>
std::string FileFrm<t_real>::GetMonVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX((mon[a-z]*[0-9]))REX", strRet, true))
		return strRet;
	return "";
}

template<class t_real>
std::string FileFrm<t_real>::GetScanCommand() const
{
	std::string strCmd;
	typename t_mapParams::const_iterator iter = m_mapParams.find("info");
	if(iter != m_mapParams.end())
		strCmd = iter->second;
	return strCmd;
}

template<class t_real>
std::string FileFrm<t_real>::GetTimestamp() const
{
	std::string strDate;
	typename t_mapParams::const_iterator iter = m_mapParams.find("file_timestamp");
	if(iter != m_mapParams.end())
		strDate = iter->second;
	return strDate;
}


// -----------------------------------------------------------------------------



template<class t_real>
void FileMacs<t_real>::ReadHeader(std::istream& istr)
{
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);

		trim(strLine);
		if(strLine.length()==0 || strLine[0]!='#')
			continue;

		strLine = strLine.substr(1);

		std::pair<std::string, std::string> pairLine =
			split_first<std::string>(strLine, " \t", 1);
		//std::cout << "key: " << pairLine.first << ", val: " << pairLine.second << std::endl;

		if(pairLine.first == "")
			continue;
		else if(pairLine.first == "Columns")
		{
			get_tokens<std::string, std::string>(pairLine.second, " \t", m_vecQuantities);
			FileInstrBase<t_real>::RenameDuplicateCols();

			//for(const std::string& strCol : m_vecQuantities)
			//	std::cout << "column: \"" << strCol << "\"" << std::endl;
			continue;
		}
		else
		{
			typename t_mapParams::iterator iter = m_mapParams.find(pairLine.first);

			if(iter == m_mapParams.end())
				m_mapParams.insert(pairLine);
			else
				iter->second += ", " + pairLine.second;
		}
	}
}

template<class t_real>
void FileMacs<t_real>::ReadData(std::istream& istr)
{
	m_vecData.resize(m_vecQuantities.size());

	// data
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);
		trim(strLine);
		if(strLine.length()==0 || strLine[0]=='#')
			continue;

		std::vector<t_real> vecToks;
		get_tokens<t_real, std::string>(strLine, " \t", vecToks);

		if(vecToks.size() != m_vecQuantities.size())
		{
			log_warn("Loader: Line size mismatch.");

			// add zeros
			while(m_vecQuantities.size() > vecToks.size())
				vecToks.push_back(0.);
		}

		for(std::size_t iTok=0; iTok<vecToks.size(); ++iTok)
			m_vecData[iTok].push_back(vecToks[iTok]);
	}
}


template<class t_real>
bool FileMacs<t_real>::Load(const char* pcFile)
{
	for(int iStep : {0,1})
	{
		std::ifstream ifstr(pcFile);
		if(!ifstr.is_open())
			return false;

		if(iStep==0)
			ReadHeader(ifstr);
		else if(iStep==1)
			ReadData(ifstr);
	}

	return true;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecVals& 
FileMacs<t_real>::GetCol(const std::string& strName, std::size_t *pIdx) const
{
	return const_cast<FileMacs*>(this)->GetCol(strName, pIdx);
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecVals& 
FileMacs<t_real>::GetCol(const std::string& strName, std::size_t *pIdx)
{
	static std::vector<t_real> vecNull;

	for(std::size_t i=0; i<m_vecQuantities.size(); ++i)
	{
		if(m_vecQuantities[i] == strName)
		{
			if(pIdx) *pIdx = i;
			return m_vecData[i];
		}
	}

	if(pIdx) *pIdx = m_vecQuantities.size();
	return vecNull;
}


template<class t_real>
std::array<t_real, 3> FileMacs<t_real>::GetSampleLattice() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Lattice");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vecToks;
	get_tokens<t_real, std::string>(iter->second, " \t", vecToks);
	if(vecToks.size() != 6)
	{
		log_err("Invalid sample lattice array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{vecToks[0], vecToks[1], vecToks[2]}};
}

template<class t_real>
std::array<t_real, 3> FileMacs<t_real>::GetSampleAngles() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Lattice");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vecToks;
	get_tokens<t_real, std::string>(iter->second, " \t", vecToks);
	if(vecToks.size() != 6)
	{
		log_err("Invalid sample lattice array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{d2r(vecToks[3]), d2r(vecToks[4]), d2r(vecToks[5])}};
}

template<class t_real>
std::array<t_real, 2> FileMacs<t_real>::GetMonoAnaD() const
{
	typename t_mapParams::const_iterator iterM = m_mapParams.find("MonoSpacing");
	typename t_mapParams::const_iterator iterA = m_mapParams.find("AnaSpacing");

	t_real m = (iterM!=m_mapParams.end() ? str_to_var<t_real>(iterM->second) : 3.355);
	t_real a = (iterA!=m_mapParams.end() ? str_to_var<t_real>(iterA->second) : 3.355);

	return std::array<t_real,2>{{m, a}};
}

template<class t_real>
std::array<bool, 3> FileMacs<t_real>::GetScatterSenses() const
{
	return std::array<bool,3>{{0, 1, 0}};
}

template<class t_real>
std::array<t_real, 3> FileMacs<t_real>::GetScatterPlane0() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Orient");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vecToks;
	get_tokens<t_real, std::string>(iter->second, " \t", vecToks);
	if(vecToks.size() != 6)
	{
		log_err("Invalid sample orientation array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{vecToks[0],vecToks[1],vecToks[2]}};
}

template<class t_real>
std::array<t_real, 3> FileMacs<t_real>::GetScatterPlane1() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("Orient");
	if(iter == m_mapParams.end())
		return std::array<t_real,3>{{0.,0.,0.}};

	std::vector<t_real> vecToks;
	get_tokens<t_real, std::string>(iter->second, " \t", vecToks);
	if(vecToks.size() != 6)
	{
		log_err("Invalid sample orientation array size.");
		return std::array<t_real,3>{{0.,0.,0.}};
	}

	return std::array<t_real,3>{{vecToks[3],vecToks[4],vecToks[5]}};
}

template<class t_real>
t_real FileMacs<t_real>::GetKFix() const
{
	// 1) look in data columns
	const std::string strKey = (IsKiFixed() ? "Ei" : "Ef");
	const t_vecVals& vecVals = GetCol(strKey);
	if(vecVals.size() != 0)
	{
		bool bImag;
		t_real k = E2k<units::si::system, t_real>
			(vecVals[0] * meV<t_real>, bImag) * angstrom<t_real>;
		return k;
	}


	// 2) look in header
	typename t_mapParams::const_iterator iter = m_mapParams.find("FixedE");
	if(iter==m_mapParams.end())
	{
		log_err("Cannot determine kfix.");
		return 0.;
	}

	std::vector<std::string> vecToks;
	get_tokens<std::string, std::string>(iter->second, " \t", vecToks);

	if(vecToks.size()<2)
	{
		log_err("Cannot determine kfix.");
		return 0.;
	}

	t_real dEfix = str_to_var<t_real>(vecToks[1]);
	bool bImag;
	t_real k = E2k<units::si::system, t_real>(dEfix * meV<t_real>, bImag) * angstrom<t_real>;
	return k;
}

template<class t_real>
bool FileMacs<t_real>::IsKiFixed() const
{
	typename t_mapParams::const_iterator iter = m_mapParams.find("FixedE");
	if(iter==m_mapParams.end())
		return 0;	// assume ckf

	std::vector<std::string> vecToks;
	get_tokens<std::string, std::string>(iter->second, " \t", vecToks);

	if(vecToks.size()==0)
		return 0;	// assume ckf

	std::string strFixedE = vecToks[0];
	trim(strFixedE);

	if(strFixedE == "Ef")
		return 0;
	else if(strFixedE == "Ei")
		return 1;

	return 0;		// assume ckf
}

template<class t_real>
std::size_t FileMacs<t_real>::GetScanCount() const
{
	if(m_vecData.size() < 1)
		return 0;
	return m_vecData[0].size();
}

template<class t_real>
std::array<t_real, 5> FileMacs<t_real>::GetScanHKLKiKf(std::size_t i) const
{
	return FileInstrBase<t_real>::GetScanHKLKiKf("QX", "QY", "QZ", "E", i);
}

template<class t_real>
bool FileMacs<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	if(!FileInstrBase<t_real>::MergeWith(pDat))
		return false;

	std::string strNr = pDat->GetScanNumber();
	if(strNr.length() != 0)
	{
		// include merged scan number
		typename t_mapParams::iterator iter = m_mapParams.find("Filename");
		if(iter != m_mapParams.end())
			iter->second += std::string(" + ") + strNr;
	}

	return true;
}


template<class t_real>
std::string FileMacs<t_real>::GetTitle() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("ExptID");
	if(iter != m_mapParams.end())
		strTitle = iter->second;

	iter = m_mapParams.find("ExptName");
	if(iter != m_mapParams.end() && iter->second != "")
		strTitle += " - " + iter->second;

	return strTitle;
}

template<class t_real>
std::string FileMacs<t_real>::GetUser() const
{
	std::string str;
	typename t_mapParams::const_iterator iter = m_mapParams.find("User");
	if(iter != m_mapParams.end())
		str = iter->second;
	return str;
}

template<class t_real>
std::string FileMacs<t_real>::GetLocalContact() const
{
	// TODO
	return "";
}


template<class t_real>
std::string FileMacs<t_real>::GetScanNumber() const
{
	std::string strTitle;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Filename");
	if(iter != m_mapParams.end())
		strTitle = iter->second;
	return strTitle;
}

template<class t_real>
std::string FileMacs<t_real>::GetSampleName() const
{
	return "";
}

template<class t_real>
std::string FileMacs<t_real>::GetSpacegroup() const
{
	return "";
}

template<class t_real>
std::vector<std::string> FileMacs<t_real>::GetScannedVars() const
{
	std::vector<std::string> vecScan;

	typename t_mapParams::const_iterator iter = m_mapParams.find("Scan");
	if(iter != m_mapParams.end())
	{
		std::vector<std::string> vecToks;
		get_tokens<std::string, std::string>(iter->second, " \t", vecToks);

		if(vecToks.size() >= 2)
			vecScan.push_back(vecToks[1]);
	}

	if(!vecScan.size())
	{
		log_warn("Could not determine scan variable.");
		if(m_vecQuantities.size() >= 1)
		{
			log_warn("Using first column: \"", m_vecQuantities[0], "\".");
			vecScan.push_back(m_vecQuantities[0]);
		}
	}

	return vecScan;
}

template<class t_real>
std::string FileMacs<t_real>::GetCountVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(spec[a-z0-9]*)REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FileMacs<t_real>::GetMonVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(mon[a-z0-9]*)REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FileMacs<t_real>::GetScanCommand() const
{
	// TODO
	return "";
}

template<class t_real>
std::string FileMacs<t_real>::GetTimestamp() const
{
	std::string str;
	typename t_mapParams::const_iterator iter = m_mapParams.find("Date");
	if(iter != m_mapParams.end())
		str = iter->second;
	return str;
}



// -----------------------------------------------------------------------------



template<class t_real>
void FileTrisp<t_real>::ReadHeader(std::istream& istr)
{
	bool bInVarSection = 0;
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);

		trim(strLine);
		if(strLine.length()==0)
			continue;

		if(str_contains<std::string>(strLine, "----", 0))	// new variable section beginning
		{
			bInVarSection = 1;
			//std::cout << "Section: " << strLine << std::endl;

			if(str_contains<std::string>(strLine, "steps", 0))
				break;
			continue;
		}

		if(bInVarSection)
		{
			std::pair<std::string, std::string> pairLine =
					split_first<std::string>(strLine, " \t", 1);

			if(pairLine.first == "")
				continue;
			//std::cout << "key: " << pairLine.first << ", val: " << pairLine.second << std::endl;

			typename t_mapParams::iterator iter = m_mapParams.find(pairLine.first);

			if(iter == m_mapParams.end())
				m_mapParams.insert(pairLine);
			else
				iter->second += ", " + pairLine.second;
		}
		else
		{
			if(begins_with<std::string>(str_to_lower(strLine), "scan start:"))
				m_mapParams["scan_start_timestamp"] = trimmed(strLine.substr(11));
			else if(begins_with<std::string>(str_to_lower(strLine), "sc"))
				m_mapParams["scan_command"] = strLine;
		}
	}
}

template<class t_real>
void FileTrisp<t_real>::ReadData(std::istream& istr)
{
	bool bAtStepsBeginning = 0;
	bool bInFooter = 0;

	// data
	while(!istr.eof())
	{
		std::string strLine;
		std::getline(istr, strLine);
		trim(strLine);

		if(!bAtStepsBeginning)
		{
			if(begins_with<std::string>(str_to_lower(strLine), "pnt"))
			{
				get_tokens<std::string, std::string>(strLine, " \t", m_vecQuantities);
				FileInstrBase<t_real>::RenameDuplicateCols();
				//for(const std::string& strCol : m_vecQuantities)
				//	std::cout << "col: " << strCol << std::endl;

				bAtStepsBeginning = 1;
				m_vecData.resize(m_vecQuantities.size());
			}
			continue;
		}

		if(strLine.length()==0 || strLine[0]=='#')
			continue;


		// character in scan data -> beginning of footer
		for(typename std::string::value_type c : split_first<std::string>(strLine, " \t", 1).first)
		{
			if(std::isalpha(c))
			{
				if(begins_with<std::string>(str_to_lower(strLine), "scan end:"))
					m_mapParams["scan_finish_timestamp"] = trimmed(strLine.substr(9));
				else if(begins_with<std::string>(str_to_lower(strLine), "scan"))
				{
					std::pair<std::string, std::string> pairLine = 
						split_first<std::string>(strLine, " \t", 1);

					m_mapParams["scan_vars"] = trimmed(pairLine.second);
				}

				bInFooter = 1;
			}
		}


		if(!bInFooter)
		{
			std::vector<t_real> vecToks;
			get_tokens<t_real, std::string>(strLine, " \t", vecToks);

			if(vecToks.size() != m_vecQuantities.size())
			{
				log_warn("Loader: Line size mismatch.");

				// add zeros
				while(m_vecQuantities.size() > vecToks.size())
					vecToks.push_back(0.);
			}

			for(std::size_t iTok=0; iTok<vecToks.size(); ++iTok)
				m_vecData[iTok].push_back(vecToks[iTok]);
		}
	}
}


template<class t_real>
bool FileTrisp<t_real>::Load(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
		return false;

	ReadHeader(ifstr);
	ReadData(ifstr);

	return true;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecVals& 
FileTrisp<t_real>::GetCol(const std::string& strName, std::size_t *pIdx) const
{
	return const_cast<FileTrisp*>(this)->GetCol(strName, pIdx);
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecVals& 
FileTrisp<t_real>::GetCol(const std::string& strName, std::size_t *pIdx)
{
	static std::vector<t_real> vecNull;

	for(std::size_t i=0; i<m_vecQuantities.size(); ++i)
	{
		if(m_vecQuantities[i] == strName)
		{
			if(pIdx) *pIdx = i;
			return m_vecData[i];
		}
	}

	if(pIdx) *pIdx = m_vecQuantities.size();
	return vecNull;
}

template<class t_real>
std::array<t_real,3> FileTrisp<t_real>::GetSampleLattice() const
{
	typename t_mapParams::const_iterator iterA = m_mapParams.find("AS");
	typename t_mapParams::const_iterator iterB = m_mapParams.find("BS");
	typename t_mapParams::const_iterator iterC = m_mapParams.find("CS");

	t_real a = (iterA!=m_mapParams.end() ? str_to_var<t_real>(iterA->second) : 0.);
	t_real b = (iterB!=m_mapParams.end() ? str_to_var<t_real>(iterB->second) : 0.);
	t_real c = (iterC!=m_mapParams.end() ? str_to_var<t_real>(iterC->second) : 0.);

	return std::array<t_real,3>{{a,b,c}};
}

template<class t_real>
std::array<t_real,3> FileTrisp<t_real>::GetSampleAngles() const
{
	typename t_mapParams::const_iterator iterA = m_mapParams.find("AA");
	typename t_mapParams::const_iterator iterB = m_mapParams.find("BB");
	typename t_mapParams::const_iterator iterC = m_mapParams.find("CC");

	t_real alpha = (iterA!=m_mapParams.end() ? d2r(str_to_var<t_real>(iterA->second)) : pi<t_real>/2.);
	t_real beta = (iterB!=m_mapParams.end() ? d2r(str_to_var<t_real>(iterB->second)) : pi<t_real>/2.);
	t_real gamma = (iterC!=m_mapParams.end() ? d2r(str_to_var<t_real>(iterC->second)) : pi<t_real>/2.);

	return std::array<t_real,3>{{alpha, beta, gamma}};
}

template<class t_real>
std::array<t_real,2> FileTrisp<t_real>::GetMonoAnaD() const
{
	typename t_mapParams::const_iterator iterM = m_mapParams.find("DM");
	typename t_mapParams::const_iterator iterA = m_mapParams.find("DA");

	t_real m = (iterM!=m_mapParams.end() ? str_to_var<t_real>(iterM->second) : 3.355);
	t_real a = (iterA!=m_mapParams.end() ? str_to_var<t_real>(iterA->second) : 3.355);

	return std::array<t_real,2>{{m, a}};
}

template<class t_real>
std::array<bool, 3> FileTrisp<t_real>::GetScatterSenses() const
{
	typename t_mapParams::const_iterator iterM = m_mapParams.find("SM");
	typename t_mapParams::const_iterator iterS = m_mapParams.find("SS");
	typename t_mapParams::const_iterator iterA = m_mapParams.find("SA");

	bool m = (iterM!=m_mapParams.end() ? (str_to_var<int>(iterM->second)>0) : 0);
	bool s = (iterS!=m_mapParams.end() ? (str_to_var<int>(iterS->second)>0) : 1);
	bool a = (iterA!=m_mapParams.end() ? (str_to_var<int>(iterA->second)>0) : 0);

	return std::array<bool,3>{{m, s, a}};
}

template<class t_real>
std::array<t_real, 3> FileTrisp<t_real>::GetScatterPlane0() const
{
	typename t_mapParams::const_iterator iterX = m_mapParams.find("AX");
	typename t_mapParams::const_iterator iterY = m_mapParams.find("AY");
	typename t_mapParams::const_iterator iterZ = m_mapParams.find("AZ");

	t_real x = (iterX!=m_mapParams.end() ? str_to_var<t_real>(iterX->second) : 1.);
	t_real y = (iterY!=m_mapParams.end() ? str_to_var<t_real>(iterY->second) : 0.);
	t_real z = (iterZ!=m_mapParams.end() ? str_to_var<t_real>(iterZ->second) : 0.);

	return std::array<t_real,3>{{x,y,z}};
}

template<class t_real>
std::array<t_real, 3> FileTrisp<t_real>::GetScatterPlane1() const
{
	typename t_mapParams::const_iterator iterX = m_mapParams.find("BX");
	typename t_mapParams::const_iterator iterY = m_mapParams.find("BY");
	typename t_mapParams::const_iterator iterZ = m_mapParams.find("BZ");

	t_real x = (iterX!=m_mapParams.end() ? str_to_var<t_real>(iterX->second) : 0.);
	t_real y = (iterY!=m_mapParams.end() ? str_to_var<t_real>(iterY->second) : 1.);
	t_real z = (iterZ!=m_mapParams.end() ? str_to_var<t_real>(iterZ->second) : 0.);

	return std::array<t_real,3>{{x,y,z}};
}

template<class t_real>
t_real FileTrisp<t_real>::GetKFix() const
{
	const std::string strKey = IsKiFixed() ? "KI" : "KF";

	typename t_mapParams::const_iterator iter = m_mapParams.find(strKey);
	if(iter==m_mapParams.end())
	{
		log_err("Cannot determine kfix.");
		return 0.;
	}

	return str_to_var<t_real>(iter->second);
}

template<class t_real>
bool FileTrisp<t_real>::IsKiFixed() const
{
	return 0;		// assume ckf
}

template<class t_real>
std::size_t FileTrisp<t_real>::GetScanCount() const
{
	if(m_vecData.size() < 1)
		return 0;
	return m_vecData[0].size();
}

template<class t_real>
std::array<t_real, 5> FileTrisp<t_real>::GetScanHKLKiKf(std::size_t i) const
{
	return FileInstrBase<t_real>::GetScanHKLKiKf("QH", "QK", "QL", "E", i);
}

template<class t_real>
bool FileTrisp<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	return FileInstrBase<t_real>::MergeWith(pDat);
}

template<class t_real> std::string FileTrisp<t_real>::GetTitle() const { return ""; }
template<class t_real> std::string FileTrisp<t_real>::GetUser() const { return ""; }
template<class t_real> std::string FileTrisp<t_real>::GetLocalContact() const { return ""; }
template<class t_real> std::string FileTrisp<t_real>::GetScanNumber() const { return ""; }
template<class t_real> std::string FileTrisp<t_real>::GetSampleName() const { return ""; }
template<class t_real> std::string FileTrisp<t_real>::GetSpacegroup() const { return ""; }

template<class t_real>
std::vector<std::string> FileTrisp<t_real>::GetScannedVars() const
{
	std::vector<std::string> vecScan;

	typename t_mapParams::const_iterator iter = m_mapParams.find("scan_vars");
	if(iter != m_mapParams.end())
		get_tokens<std::string, std::string>(iter->second, " \t", vecScan);

	if(!vecScan.size())
	{
		log_warn("Could not determine scan variable.");
		if(m_vecQuantities.size() >= 1)
		{
			log_warn("Using first column: \"", m_vecQuantities[0], "\".");
			vecScan.push_back(m_vecQuantities[0]);
		}
	}

	return vecScan;
}

template<class t_real>
std::string FileTrisp<t_real>::GetCountVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(c[0-9])REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FileTrisp<t_real>::GetMonVar() const
{
	std::string strRet;
	if(FileInstrBase<t_real>::MatchColumn(R"REX(mon[a-z0-9]*)REX", strRet))
		return strRet;
	return "";
}

template<class t_real>
std::string FileTrisp<t_real>::GetScanCommand() const
{
	std::string str;
	typename t_mapParams::const_iterator iter = m_mapParams.find("scan_command");
	if(iter != m_mapParams.end())
		str = iter->second;
	return str;
}

template<class t_real>
std::string FileTrisp<t_real>::GetTimestamp() const
{
	std::string str;
	typename t_mapParams::const_iterator iter = m_mapParams.find("scan_start_timestamp");
	if(iter != m_mapParams.end())
		str = iter->second;
	return str;
}



// -----------------------------------------------------------------------------


template<class t_real>
bool FileRaw<t_real>::Load(const char* pcFile)
{
	bool bOk = m_dat.Load(pcFile);
	m_vecCols.clear();
	for(std::size_t iCol=0; iCol<m_dat.GetColumnCount(); ++iCol)
		m_vecCols.emplace_back(var_to_str(iCol+1));
	return bOk;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecVals& 
FileRaw<t_real>::GetCol(const std::string& strName, std::size_t *pIdx) const
{
	return const_cast<FileRaw*>(this)->GetCol(strName, pIdx);
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecVals& 
FileRaw<t_real>::GetCol(const std::string& strName, std::size_t *pIdx)
{
	std::size_t iCol = str_to_var<std::size_t>(strName)-1;
	if(iCol < m_dat.GetColumnCount())
	{
		if(pIdx) *pIdx = iCol;
		return m_dat.GetColumn(iCol);
	}

	static std::vector<t_real> vecNull;
	if(pIdx) *pIdx = m_dat.GetColumnCount();
	return vecNull;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecDat&
FileRaw<t_real>::GetData() const
{
	return m_dat.GetData();
}

template<class t_real>
typename FileInstrBase<t_real>::t_vecDat&
FileRaw<t_real>::GetData()
{
	return m_dat.GetData();
}

template<class t_real>
const typename FileInstrBase<t_real>::t_vecColNames& 
FileRaw<t_real>::GetColNames() const
{
	return m_vecCols;
}

template<class t_real>
const typename FileInstrBase<t_real>::t_mapParams& 
FileRaw<t_real>::GetAllParams() const 
{
	return m_dat.GetHeader();
}

template<class t_real>
std::array<t_real,3> FileRaw<t_real>::GetSampleLattice() const
{
	return std::array<t_real,3>{{0.,0.,0.}};
}

template<class t_real>
std::array<t_real,3> FileRaw<t_real>::GetSampleAngles() const
{
	return std::array<t_real,3>{{0., 0., 0.}};
}

template<class t_real>
std::array<t_real,2> FileRaw<t_real>::GetMonoAnaD() const
{
	return std::array<t_real,2>{{0., 0.}};
}

template<class t_real>
std::array<bool, 3> FileRaw<t_real>::GetScatterSenses() const
{
	return std::array<bool,3>{{0, 0, 0}};
}

template<class t_real>
std::array<t_real, 3> FileRaw<t_real>::GetScatterPlane0() const
{
	return std::array<t_real,3>{{0.,0.,0.}};
}

template<class t_real>
std::array<t_real, 3> FileRaw<t_real>::GetScatterPlane1() const
{
	return std::array<t_real,3>{{0.,0.,0.}};
}

template<class t_real>
t_real FileRaw<t_real>::GetKFix() const
{
	return 0.;
}

template<class t_real>
bool FileRaw<t_real>::IsKiFixed() const
{
	return 0;
}

template<class t_real>
std::size_t FileRaw<t_real>::GetScanCount() const
{
	if(m_dat.GetColumnCount() != 0)
		return m_dat.GetRowCount();
	return 0;
}

template<class t_real>
std::array<t_real, 5> FileRaw<t_real>::GetScanHKLKiKf(std::size_t i) const
{
	return std::array<t_real,5>{{0.,0.,0.,0.,0.}};
}

template<class t_real>
bool FileRaw<t_real>::MergeWith(const FileInstrBase<t_real>* pDat)
{
	return 0;
}

template<class t_real> std::string FileRaw<t_real>::GetTitle() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetUser() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetLocalContact() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetScanNumber() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetSampleName() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetSpacegroup() const { return ""; }
template<class t_real> std::vector<std::string> FileRaw<t_real>::GetScannedVars() const { return {"1"}; }
template<class t_real> std::string FileRaw<t_real>::GetCountVar() const { return "2"; }
template<class t_real> std::string FileRaw<t_real>::GetMonVar() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetScanCommand() const { return ""; }
template<class t_real> std::string FileRaw<t_real>::GetTimestamp() const { return ""; }

}


#endif
