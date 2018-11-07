/**
 * tlibs2
 * fitting and interpolation library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date apr-2012 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_FITTER_H__
#define __TLIBS2_FITTER_H__

#include <Minuit2/FCNBase.h>
#include <Minuit2/MnFcn.h>
#include <Minuit2/FunctionMinimum.h>
#include <Minuit2/MnMigrad.h>
#include <Minuit2/MnPrint.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/math/special_functions/binomial.hpp>

#include "math.h"
#include "log.h"
#include "traits.h"


namespace tl2 {

using t_real_min = typename std::result_of<
	decltype(&ROOT::Minuit2::MnFcn::Up)(ROOT::Minuit2::MnFcn)>::type;


// ----------------------------------------------------------------------------
// function models

template<class t_real>
class FitterFuncModel
{
public:
	virtual ~FitterFuncModel() = default;

	virtual bool SetParams(const std::vector<t_real>& vecParams) = 0;
	virtual t_real operator()(t_real x) const = 0;
	virtual FitterFuncModel<t_real>* copy() const = 0;
};


/**
 * interface using supplied functions
 * iNumArgs also includes the "x" parameter to the function, m_vecVals does not
 */
template<class t_real, std::size_t iNumArgs, typename t_func>
class FitterLamFuncModel : public FitterFuncModel<t_real>
{
protected:
	t_func m_func;
	std::vector<t_real> m_vecVals;
	bool m_bSeparateFreeParam = 1;	// separate "x" from parameters (for fitter)

public:
	FitterLamFuncModel(t_func func, bool bSeparateX=1)
		: m_func(func), m_bSeparateFreeParam(bSeparateX)
	{
		m_vecVals.resize(m_bSeparateFreeParam ? iNumArgs-1 : iNumArgs);
	}

	virtual bool SetParams(const std::vector<t_real>& vecParams) override
	{
		for(std::size_t i=0; i<std::min(vecParams.size(), m_vecVals.size()); ++i)
			m_vecVals[i] = vecParams[i];
		return true;
	}

	virtual t_real operator()(t_real x = t_real(0)) const override
	{
		std::vector<t_real> vecValsWithX;
		if(m_bSeparateFreeParam)
		{
			vecValsWithX.push_back(x);
			for(t_real d : m_vecVals) vecValsWithX.push_back(d);
		}

		const std::vector<t_real> *pvecVals = m_bSeparateFreeParam ? &vecValsWithX : &m_vecVals;
		return call<iNumArgs, t_func, t_real, std::vector>(m_func, *pvecVals);
	}

	virtual FitterLamFuncModel* copy() const override
	{
		FitterLamFuncModel<t_real, iNumArgs, t_func>* pMod =
			new FitterLamFuncModel<t_real, iNumArgs, t_func>(m_func);

		pMod->m_vecVals = this->m_vecVals;
		pMod->m_bSeparateFreeParam = this->m_bSeparateFreeParam;

		return pMod;
	}
};

// ----------------------------------------------------------------------------



/**
 * generic chi^2 calculation for fitting
 */
template<class t_real = t_real_min>
class Chi2Function : public ROOT::Minuit2::FCNBase
{
protected:
	const FitterFuncModel<t_real_min> *m_pfkt = nullptr;

	std::size_t m_uiLen;
	const t_real* m_px = nullptr;
	const t_real* m_py = nullptr;
	const t_real* m_pdy = nullptr;

	t_real_min m_dSigma = 1.;
	bool m_bDebug = 0;

public:
	Chi2Function(const FitterFuncModel<t_real_min>* fkt=0,
		std::size_t uiLen=0, const t_real *px=0,
		const t_real *py=0, const t_real *pdy=0)
		: m_pfkt(fkt), m_uiLen(uiLen), m_px(px), m_py(py), m_pdy(pdy)
	{}

	virtual ~Chi2Function() = default;

	/*
	 * chi^2 calculation
	 * based on the example in the Minuit user's guide:
	 * http://seal.cern.ch/documents/minuit/mnusersguide.pdf
	 */
	t_real_min chi2(const std::vector<t_real_min>& vecParams) const
	{
		// cannot operate on m_pfkt directly, because Minuit
		// uses more than one thread!
		std::unique_ptr<FitterFuncModel<t_real_min>> uptrFkt(m_pfkt->copy());
		FitterFuncModel<t_real_min>* pfkt = uptrFkt.get();

		pfkt->SetParams(vecParams);
		return tl2::chi2<t_real_min, decltype(*pfkt), const t_real*>(*pfkt, m_uiLen, m_px, m_py, m_pdy);
	}

	virtual t_real_min Up() const override { return m_dSigma*m_dSigma; }

	virtual t_real_min operator()(const std::vector<t_real_min>& vecParams) const override
	{
		t_real_min dChi2 = chi2(vecParams);
		if(m_bDebug) log_debug("Chi2 = ", dChi2);
		return dChi2;
	}

	void SetSigma(t_real_min dSig) { m_dSigma = dSig; }
	t_real_min GetSigma() const { return m_dSigma; }

	void SetDebug(bool b) { m_bDebug = b; }
};


/**
 * function adaptor for minimisation
 */
template<class t_real = t_real_min>
class MiniFunction : public ROOT::Minuit2::FCNBase
{
protected:
	const FitterFuncModel<t_real_min> *m_pfkt = nullptr;
	t_real_min m_dSigma = 1.;

public:
	MiniFunction(const FitterFuncModel<t_real_min>* fkt=0) : m_pfkt(fkt) {}
	virtual ~MiniFunction() = default;

	virtual t_real_min Up() const override { return m_dSigma*m_dSigma; }

	virtual t_real_min operator()(const std::vector<t_real_min>& vecParams) const override
	{
		// cannot operate on m_pfkt directly, because Minuit
		// uses more than one thread!
		std::unique_ptr<FitterFuncModel<t_real_min>> uptrFkt(m_pfkt->copy());
		FitterFuncModel<t_real_min>* pfkt = uptrFkt.get();

		pfkt->SetParams(vecParams);
		return (*pfkt)(t_real_min(0));	// "0" is an ignored dummy value here
	}

	void SetSigma(t_real_min dSig) { m_dSigma = dSig; }
	t_real_min GetSigma() const { return m_dSigma; }
};



// ----------------------------------------------------------------------------


/**
 * fit function to x,y,dy data points
 */
template<std::size_t iNumArgs, typename t_func>
bool fit(t_func&& func,

	const std::vector<t_real_min>& vecX,
	const std::vector<t_real_min>& vecY,
	const std::vector<t_real_min>& vecYErr,

	const std::vector<std::string>& vecParamNames,	// size: iNumArgs-1
	std::vector<t_real_min>& vecVals,
	std::vector<t_real_min>& vecErrs,
	const std::vector<bool>* pVecFixed = nullptr,

	bool bDebug=1) noexcept
{
	try
	{
		if(!vecX.size() || !vecY.size() || !vecYErr.size())
		{
			log_err("No data given to fitter.");
			return false;
		}

		// check if all params are fixed
		if(pVecFixed && std::all_of(pVecFixed->begin(), pVecFixed->end(),
			[](bool b)->bool { return b; }))
			{
				log_err("All parameters are fixed.");
				return false;
			}

		FitterLamFuncModel<t_real_min, iNumArgs, t_func> mod(func);
		Chi2Function<t_real_min> chi2(&mod, vecX.size(), vecX.data(), vecY.data(), vecYErr.data());

		ROOT::Minuit2::MnUserParameters params;
		for(std::size_t iParam=0; iParam<vecParamNames.size(); ++iParam)
		{
			params.Add(vecParamNames[iParam], vecVals[iParam], vecErrs[iParam]);
			if(pVecFixed && (*pVecFixed)[iParam])
				params.Fix(vecParamNames[iParam]);
		}

		ROOT::Minuit2::MnMigrad migrad(chi2, params, 2);
		ROOT::Minuit2::FunctionMinimum mini = migrad();
		bool bValidFit = mini.IsValid() && mini.HasValidParameters() && mini.UserState().IsValid();

		for(std::size_t iParam=0; iParam<vecParamNames.size(); ++iParam)
		{
			vecVals[iParam] = mini.UserState().Value(vecParamNames[iParam]);
			vecErrs[iParam] = std::fabs(mini.UserState().Error(vecParamNames[iParam]));
		}

		if(bDebug)
			log_debug(mini);

		return bValidFit;
	}
	catch(const std::exception& ex)
	{
		log_err(ex.what());
	}

	return false;
}


/**
 * find function minimum
 */
template<std::size_t iNumArgs, typename t_func>
bool minimise(t_func&& func, const std::vector<std::string>& vecParamNames,
	std::vector<t_real_min>& vecVals, std::vector<t_real_min>& vecErrs,
	const std::vector<bool>* pVecFixed = nullptr, bool bDebug=1) noexcept
{
	try
	{
		// check if all params are fixed
		if(pVecFixed && std::all_of(pVecFixed->begin(), pVecFixed->end(),
			[](bool b)->bool { return b; }))
			{
				log_err("All parameters are fixed.");
				return false;
			}

		FitterLamFuncModel<t_real_min, iNumArgs, t_func> mod(func, false);
		MiniFunction<t_real_min> chi2(&mod);

		ROOT::Minuit2::MnUserParameters params;
		for(std::size_t iParam=0; iParam<vecParamNames.size(); ++iParam)
		{
			params.Add(vecParamNames[iParam], vecVals[iParam], vecErrs[iParam]);
			if(pVecFixed && (*pVecFixed)[iParam])
				params.Fix(vecParamNames[iParam]);
		}

		ROOT::Minuit2::MnMigrad migrad(chi2, params, 2);
		ROOT::Minuit2::FunctionMinimum mini = migrad();
		bool bMinimumValid = mini.IsValid() && mini.HasValidParameters() && mini.UserState().IsValid();

		for(std::size_t iParam=0; iParam<vecParamNames.size(); ++iParam)
		{
			vecVals[iParam] = mini.UserState().Value(vecParamNames[iParam]);
			vecErrs[iParam] = std::fabs(mini.UserState().Error(vecParamNames[iParam]));
		}

		if(bDebug)
			log_debug(mini);

		return bMinimumValid;
	}
	catch(const std::exception& ex)
	{
		log_err(ex.what());
	}

	return false;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// interpolation

/**
 * see: http://mathworld.wolfram.com/BernsteinPolynomial.html
 */
template<typename T> T bernstein(int i, int n, T t)
{
	T bino = boost::math::binomial_coefficient<T>(n, i);
	return bino * pow(t, i) * pow(1-t, n-i);
}

/**
 * see: http://mathworld.wolfram.com/BezierCurve.html
 */
template<typename T>
ublas::vector<T> bezier(const ublas::vector<T>* P, std::size_t N, T t)
{
	if(N==0) return ublas::vector<T>(0);
	const int n = N-1;

	ublas::vector<T> vec(P[0].size());
	for(std::size_t i=0; i<vec.size(); ++i) vec[i] = T(0);

	for(int i=0; i<=n; ++i)
		vec += P[i]*bernstein(i, n, t);

	return vec;
}


/**
 * see: http://mathworld.wolfram.com/B-Spline.html
 */
template<typename T>
T bspline_base(int i, int j, T t, const std::vector<T>& knots)
{
	if(j==0)
	{
		if((knots[i] <= t) && (t < knots[i+1]) && (knots[i]<knots[i+1]))
			return 1.;
		return 0.;
	}

	T val11 = (t - knots[i]) / (knots[i+j]-knots[i]);
	T val12 = bspline_base(i, j-1, t, knots);
	T val1 = val11 * val12;

	T val21 = (knots[i+j+1]-t) / (knots[i+j+1]-knots[i+1]);
	T val22 = bspline_base(i+1, j-1, t, knots);
	T val2 = val21 * val22;

	T val = val1 + val2;
	return val;
}


/**
 * see: http://mathworld.wolfram.com/B-Spline.html
 */
template<typename T>
ublas::vector<T> bspline(const ublas::vector<T>* P, std::size_t N, T t, const std::vector<T>& knots)
{
	if(N==0) return ublas::vector<T>(0);
	const int n = N-1;
	const int m = knots.size()-1;
	const int degree = m-n-1;

	ublas::vector<T> vec(P[0].size());
	for(std::size_t i=0; i<vec.size(); ++i)
		vec[i] = T(0);

	for(int i=0; i<=n; ++i)
		vec += P[i]*bspline_base(i, degree, t, knots);

	return vec;
}


// ----------------------------------------------------------------------------

template<typename T=double>
class Bezier
{
	protected:
		std::unique_ptr<ublas::vector<T>[]> m_pvecs;
		std::size_t m_iN;

	public:
		Bezier(std::size_t N, const T *px, const T *py) : m_iN(N)
		{
			m_pvecs.reset(new ublas::vector<T>[m_iN]);

			for(std::size_t i=0; i<m_iN; ++i)
			{
				m_pvecs[i].resize(2);
				m_pvecs[i][0] = px[i];
				m_pvecs[i][1] = py[i];
			}
		}


		ublas::vector<T> operator()(T t) const
		{
			return bezier<T>(m_pvecs.get(), m_iN, t);
		}
};


template<typename T=double>
class BSpline
{
	protected:
		std::unique_ptr<ublas::vector<T>[]> m_pvecs;
		std::size_t m_iN, m_iDegree;
		std::vector<T> m_vecKnots;

	public:
		BSpline(std::size_t N, const T *px, const T *py, unsigned int iDegree=3) : m_iN(N), m_iDegree(iDegree)
		{
			m_pvecs.reset(new ublas::vector<T>[m_iN]);

			for(std::size_t i=0; i<m_iN; ++i)
			{
				m_pvecs[i].resize(2);
				m_pvecs[i][0] = px[i];
				m_pvecs[i][1] = py[i];
			}

			std::size_t iM = m_iDegree + m_iN + 1;
			m_vecKnots.resize(iM);

			const T eps = std::numeric_limits<T>::epsilon();

			// set knots to uniform, nonperiodic B-Spline
			for(unsigned int i=0; i<m_iDegree+1; ++i)
				m_vecKnots[i] = 0.+i*eps;
			for(unsigned int i=iM-m_iDegree-1; i<iM; ++i)
				m_vecKnots[i] = 1.-i*eps;
			for(unsigned int i=m_iDegree+1; i<iM-m_iDegree-1; ++i)
				m_vecKnots[i] = T(i+1-m_iDegree-1) / T(iM-2*m_iDegree-2 + 1);
		}


		ublas::vector<T> operator()(T t) const
		{
			if(m_iN==0)
			{
				ublas::vector<T> vecNull(2);
				vecNull[0] = vecNull[1] = 0.;
				return vecNull;
			}

			ublas::vector<T> vec = bspline<T>(m_pvecs.get(), m_iN, t, m_vecKnots);

			// remove epsilon dependence
			if(t<=0.) vec = m_pvecs[0];
			if(t>=1.) vec = m_pvecs[m_iN-1];

			return vec;
		}
};


template<typename T=double>
class LinInterp
{
protected:
	std::unique_ptr<ublas::vector<T>[]> m_pvecs;
	std::size_t m_iN;

public:
	LinInterp(std::size_t N, const T *px, const T *py) : m_iN(N)
	{
		m_pvecs.reset(new ublas::vector<T>[m_iN]);

		for(std::size_t i=0; i<m_iN; ++i)
		{
			m_pvecs[i].resize(2);
			m_pvecs[i][0] = px[i];
			m_pvecs[i][1] = py[i];
		}

		// ensure that vector is sorted by x values
		std::stable_sort(m_pvecs.get(), m_pvecs.get()+m_iN,
			[](const ublas::vector<T>& vec1, const ublas::vector<T>& vec2) -> bool
			{ return vec1[0] < vec2[0]; });
	}


	T operator()(T x) const
	{
		const auto* iterBegin = m_pvecs.get();
		const auto* iterEnd = m_pvecs.get() + m_iN;

		if(m_iN == 0) return T(0);
		if(m_iN == 1) return (*iterBegin)[1];

		const auto* iterLower = std::lower_bound(iterBegin, iterEnd, x,
			[](const ublas::vector<T>& vec, const T& x) -> bool
			{ return vec[0] < x; });

		// lower bound at end of range?
		if(iterLower == iterEnd || iterLower == iterEnd-1)
			iterLower = iterEnd - 2;
		const auto* iter2 = iterLower + 1;

		T xrange = (*iter2)[0] - (*iterLower)[0];
		T xpos = (x-(*iterLower)[0]) / xrange;

		return lerp<T,T>((*iterLower)[1], (*iter2)[1], xpos);
	}
};



// ----------------------------------------------------------------------------

}

#endif
