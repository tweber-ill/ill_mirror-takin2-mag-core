/**
 * structure factor calculation test
 * @author Tobias Weber <tweber@ill.fr>
 * @date 18-mar-18
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 *
 * g++ -std=c++17 -fconcepts -I../../ -o structurefactor structurefactor.cpp
 */

#include <boost/algorithm/string.hpp>
#include <boost/units/systems/si/codata/electron_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
namespace si = boost::units::si;
namespace consts = si::constants;

#include <iostream>
#include <fstream>
#include <memory>

#include "tlibs2/libs/math20.h"
using namespace tl2;
using namespace tl2_ops;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_real>;
using t_mat = mat<t_real, std::vector>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat_cplx = mat<t_cplx, std::vector>;

std::string g_ws = " \t";
const t_real g_eps = 1e-6;


template<class T>
T from_str(const std::string& str)
{
	T t;

	std::istringstream istr(str);
	istr >> t;

	return t;
}


struct PowderLine
{
	t_real Q;
	t_real I;
	std::string peaks;
};


void calc(std::istream& istr)
{
	std::vector<t_vec_cplx> Ms;
	std::vector<t_cplx> bs;
	std::vector<t_vec> Rs;

	t_real latt[3] = {5., 5., 5.};
	t_real angle[3] = {90., 90., 90.};

	bool bNucl = 1;
	bool bPowder = 0;
	std::size_t linenr = 0;

	// magnetic propagation vector
	auto prop = create<t_vec>({0,0,0});

	while(istr)
	{
		std::string line;
		std::getline(istr, line);
		++linenr;

		boost::trim_if(line, boost::is_any_of(g_ws));
		if(line == "")
			continue;

		std::vector<std::string> vectoks;
		boost::split(vectoks, line, boost::is_any_of(g_ws), boost::token_compress_on);

		if(vectoks.size() == 4)		// nuclear
		{
			// atomic position
			t_real Rx = from_str<t_real>(vectoks[0]);
			t_real Ry = from_str<t_real>(vectoks[1]);
			t_real Rz = from_str<t_real>(vectoks[2]);

			// scattering length
			t_cplx b = from_str<t_cplx>(vectoks[3]);

			Rs.emplace_back(create<t_vec>({Rx, Ry, Rz}));
			bs.emplace_back(b);
			bNucl = 1;
		}
		else if(vectoks.size() == 6)	// magnetic
		{
			// atomic position
			t_real Rx = from_str<t_real>(vectoks[0]);
			t_real Ry = from_str<t_real>(vectoks[1]);
			t_real Rz = from_str<t_real>(vectoks[2]);

			// magnetic moment
			// TODO: length needed if vector is scaled to 1!
			t_real Mx = from_str<t_real>(vectoks[3]);
			t_real My = from_str<t_real>(vectoks[4]);
			t_real Mz = from_str<t_real>(vectoks[5]);

			Rs.emplace_back(create<t_vec>({Rx, Ry, Rz}));
			Ms.emplace_back(create<t_vec_cplx>({Mx, My, Mz}));
			bNucl = 0;
		}
		else if(vectoks.size() == 8 && vectoks[0] == "x")	// unit cell definition
		{
			latt[0] = from_str<t_real>(vectoks[1]);
			latt[1] = from_str<t_real>(vectoks[2]);
			latt[2] = from_str<t_real>(vectoks[3]);
			angle[0] = from_str<t_real>(vectoks[4]);
			angle[1] = from_str<t_real>(vectoks[5]);
			angle[2] = from_str<t_real>(vectoks[6]);
			bPowder = (from_str<int>(vectoks[7]) != 0);
		}
		else if(vectoks.size() == 3 /*&& vectoks[0] == "k"*/)	// propagation vector k
		{
			prop[0] = from_str<t_real>(vectoks[0]);
			prop[1] = from_str<t_real>(vectoks[1]);
			prop[2] = from_str<t_real>(vectoks[2]);
		}
		else
		{
			std::cerr << "Error in line " << linenr << "." << std::endl;
			continue;
		}
	}


	//auto crystB = unit<t_mat>(3);
	auto crystB = B_matrix<t_mat>(latt[0], latt[1], latt[2],
		angle[0]/180.*pi<t_real>, angle[1]/180.*pi<t_real>, angle[2]/180.*pi<t_real>);


	std::size_t prec = 6;
	std::cout.precision(prec);
	std::cout << "Crystal lattice: a = " << latt[0] << ", b = " << latt[1] << ", c = " << latt[2]
		<< ", alpha = " << angle[0] << ", beta = " << angle[1] << ", gamma = " << angle[2] << "\n";
	std::cout << "Crystal matrix: B = " << crystB << "\n";
	std::cout << Rs.size() << " atom(s) defined.\n";
	std::cout << bs.size() << " scattering length(s) defined.\n";
	std::cout << Ms.size() << " magnetic moment(s) defined.\n";
	std::cout << "Magnetic propagation vector: k = " << prop << ".\n";

	const t_real maxBZ = 5.;
	const t_real p = -t_real(consts::codata::mu_n/consts::codata::mu_N*consts::codata::r_e/si::meters)*0.5e15;
	//std::cout << "p = " << p << "\n";

	if(!bPowder)
	{
		if(bNucl)
		{
			std::cout << "Nuclear single-crystal structure factors:" << "\n";
			std::cout
				<< std::setw(prec*1.5) << std::right << "h (rlu)" << " "
				<< std::setw(prec*1.5) << std::right << "k (rlu)" << " "
				<< std::setw(prec*1.5) << std::right << "l (rlu)" << " "
				<< std::setw(prec*2) << std::right << "|Q| (1/A)" << " "
				<< std::setw(prec*2) << std::right << "|Fn|^2" << " "
				<< std::setw(prec*5) << std::right << "Fn (fm)" << "\n";
		}
		else
		{
			std::cout << "Magnetic single-crystal structure factors:" << "\n";
			std::cout
				<< std::setw(prec*2) << std::right << "h (rlu)" << " "
				<< std::setw(prec*2) << std::right << "k (rlu)" << " "
				<< std::setw(prec*2) << std::right << "l (rlu)" << " "
				<< std::setw(prec*2) << std::right << "|Q| (1/A)" << " "
				<< std::setw(prec*2) << std::right << "|Fm|^2" << " "
				<< std::setw(prec*2) << std::right << "|Fm_perp|^2" << " "
				<< std::setw(prec*5) << std::right << "Fm_x (fm)" << " "
				<< std::setw(prec*5) << std::right << "Fm_y (fm)" << " "
				<< std::setw(prec*5) << std::right << "Fm_z (fm)" << " "
				<< std::setw(prec*5) << std::right << "Fm_perp_x (fm)" << " "
				<< std::setw(prec*5) << std::right << "Fm_perp_y (fm)" << " "
				<< std::setw(prec*5) << std::right << "Fm_perp_z (fm)" << "\n";
		}
	}
	else
	{
		if(bNucl)
			std::cout << "Nuclear powder lines:" << "\n";
		else
			std::cout << "Magnetic powder lines:" << "\n";

		std::cout
			<< std::setw(prec*2) << std::right << "|Q| (1/A)" << " "
			<< std::setw(prec*2) << std::right << "|F|^2" << "\n";
	}


	// TODO
	auto calc_magformfact = [](std::size_t atomidx, const auto& Qvec_invA) -> t_real
	{
		return 1.;
	};


	std::vector<PowderLine> powderlines;
	auto add_powderline = [&powderlines](t_real Q, t_real I,
		t_real h, t_real k, t_real l)
	{
		std::ostringstream ostrPeak;
		ostrPeak << "(" << h << "," << k << "," << l << ");";

		// is this Q value already in the vector?
		bool foundQ = false;
		for(auto& line : powderlines)
		{
			if(equals<t_real>(line.Q, Q, g_eps))
			{
				line.I += I;
				line.peaks +=  ostrPeak.str();
				foundQ = true;
				break;
			}
		}

		// start a new line
		if(!foundQ)
		{
			PowderLine line;
			line.Q = Q;
			line.I = I;
			line.peaks = ostrPeak.str();
			powderlines.emplace_back(std::move(line));
		}
	};


	for(t_real h=-maxBZ; h<=maxBZ; ++h)
		for(t_real k=-maxBZ; k<=maxBZ; ++k)
			for(t_real l=-maxBZ; l<=maxBZ; ++l)
			{
				auto Q = create<t_vec>({h,k,l}) + prop;
				auto Q_invA = crystB * Q;
				auto Qabs_invA = norm(Q_invA);

				if(bNucl)
				{
					// nuclear structure factor
					auto Fn = structure_factor<t_vec, t_cplx>(bs, Rs, Q);
					//if(equals<t_cplx>(Fn, t_cplx(0), g_eps)) Fn = 0.;
					if(equals<t_real>(Fn.real(), 0, g_eps)) Fn.real(0.);
					if(equals<t_real>(Fn.imag(), 0, g_eps)) Fn.imag(0.);
					auto I = (std::conj(Fn)*Fn).real();

					if(!bPowder)
					{
						std::cout
							<< std::setw(prec*1.5) << std::right << h << " "
							<< std::setw(prec*1.5) << std::right << k << " "
							<< std::setw(prec*1.5) << std::right << l << " "
							<< std::setw(prec*2) << std::right << Qabs_invA << " "
							<< std::setw(prec*2) << std::right << I << " "
							<< std::setw(prec*5) << std::right << Fn << "\n";
					}
					else
					{
						add_powderline(Qabs_invA, I, h,k,l);
					}
				}
				else
				{
					// magnetic form factors
					std::vector<t_real> fs;
					for(std::size_t atomidx=0; atomidx<Ms.size(); ++atomidx)
						fs.push_back(calc_magformfact(atomidx, Q_invA));

					// magnetic structure factor
					auto Fm = p * structure_factor<t_vec, t_vec_cplx>(Ms, Rs, Q, &fs);
					for(auto &comp : Fm)
						if(equals<t_cplx>(comp, t_cplx(0), g_eps))
							comp = 0.;

					// neutron scattering: orthogonal projection onto plane with normal Q.
					auto Fm_perp = ortho_project<t_vec_cplx>(
						Fm, create<t_vec_cplx>({Q[0], Q[1], Q[2]}), false);

					t_real I = (std::conj(Fm[0])*Fm[0] +
						std::conj(Fm[1])*Fm[1] +
						std::conj(Fm[2])*Fm[2]).real();
					t_real I_perp = (std::conj(Fm_perp[0])*Fm_perp[0] +
						std::conj(Fm_perp[1])*Fm_perp[1] +
						std::conj(Fm_perp[2])*Fm_perp[2]).real();

					if(!bPowder)
					{
						std::cout
							<< std::setw(prec*2) << std::right << h+prop[0] << " "
							<< std::setw(prec*2) << std::right << k+prop[1] << " "
							<< std::setw(prec*2) << std::right << l+prop[2] << " "
							<< std::setw(prec*2) << std::right << Qabs_invA << " "
							<< std::setw(prec*2) << std::right << I << " "
							<< std::setw(prec*2) << std::right << I_perp << " "
							<< std::setw(prec*5) << std::right << Fm[0] << " "
							<< std::setw(prec*5) << std::right << Fm[1] << " "
							<< std::setw(prec*5) << std::right << Fm[2] << " "
							<< std::setw(prec*5) << std::right << Fm_perp[0] << " "
							<< std::setw(prec*5) << std::right << Fm_perp[1] << " "
							<< std::setw(prec*5) << std::right << Fm_perp[2] << "\n";
					}
					else
					{
						add_powderline(Qabs_invA, I_perp, h+prop[0],k+prop[1],l+prop[2]);
					}
				}
			}


	if(bPowder)
	{
		std::stable_sort(powderlines.begin(), powderlines.end(),
			[](const PowderLine& line1, const PowderLine& line2) -> bool
			{
				return line1.Q < line2.Q;
			});

		for(const auto& line : powderlines)
		{
			std::cout
				<< std::setw(prec*2) << std::right << line.Q << " "
				<< std::setw(prec*2) << std::right << line.I << " "
				<< line.peaks << "\n";
		}
	}
}


int main(int argc, char **argv)
{
	std::istream* pIstr = &std::cin;

	std::unique_ptr<std::ifstream> ifstr;
	if(argc > 1)
	{
		ifstr.reset(new std::ifstream(argv[1]));
		pIstr = ifstr.get();
	}

	calc(*pIstr);
	return 0;
}
