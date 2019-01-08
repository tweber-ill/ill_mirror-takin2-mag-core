/**
 * Calculation of polarisation vector
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#include <QtCore/QSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTabWidget>

#include <locale>
#include <iostream>
#include <vector>
#include <string>

#include "../glplot/glplot.h"
#include "libs/_cxx20/math_algos.h"
#include "libs/helper.h"

#include <boost/version.hpp>
#include <boost/config.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;


using namespace m_ops;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_cplx>;
using t_mat = m::mat<t_cplx, std::vector>;
using t_matvec = std::vector<t_mat>;


// ----------------------------------------------------------------------------
class PolDlg : public QDialog
{ /*Q_OBJECT*/
private:
	QSettings m_sett{"tobis_stuff", "pol"};

	std::shared_ptr<GlPlot> m_plot{std::make_shared<GlPlot>(this)};
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };

	QLineEdit* m_editNRe = new QLineEdit("0", this);
	QLineEdit* m_editNIm = new QLineEdit("0", this);

	QLineEdit* m_editMPerpReX = new QLineEdit("0", this);
	QLineEdit* m_editMPerpReY = new QLineEdit("1", this);
	QLineEdit* m_editMPerpReZ = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImX = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImY = new QLineEdit("0", this);
	QLineEdit* m_editMPerpImZ = new QLineEdit("0", this);

	QLineEdit* m_editPiX = new QLineEdit("1", this);
	QLineEdit* m_editPiY = new QLineEdit("0", this);
	QLineEdit* m_editPiZ = new QLineEdit("0", this);
	QLineEdit* m_editPfX = new QLineEdit(this);
	QLineEdit* m_editPfY = new QLineEdit(this);
	QLineEdit* m_editPfZ = new QLineEdit(this);

	QLabel* m_labelStatus = new QLabel(this);


	// 3d object handles
	std::size_t m_arrow_pi = 0;
	std::size_t m_arrow_pf = 0;
	std::size_t m_arrow_M_Re = 0;
	std::size_t m_arrow_M_Im = 0;

	bool m_3dobjsReady = false;
	bool m_mouseDown[3] = {false, false, false};
	long m_curPickedObj = -1;
	long m_curDraggedObj = -1;


protected:
	virtual void closeEvent(QCloseEvent *evt) override
	{
		// save window size and position
		m_sett.setValue("geo", saveGeometry());

		// save values
		m_sett.setValue("n_re", m_editNRe->text().toDouble());
		m_sett.setValue("n_im", m_editNIm->text().toDouble());
		m_sett.setValue("mx_re", m_editMPerpReX->text().toDouble());
		m_sett.setValue("my_re", m_editMPerpReY->text().toDouble());
		m_sett.setValue("mz_re", m_editMPerpReZ->text().toDouble());
		m_sett.setValue("mx_im", m_editMPerpImX->text().toDouble());
		m_sett.setValue("my_im", m_editMPerpImY->text().toDouble());
		m_sett.setValue("mz_im", m_editMPerpImZ->text().toDouble());
		m_sett.setValue("pix", m_editPiX->text().toDouble());
		m_sett.setValue("piy", m_editPiY->text().toDouble());
		m_sett.setValue("piz", m_editPiZ->text().toDouble());
	}


protected slots:
	/**
	 * called after the plotter has initialised
	 */
	void AfterGLInitialisation()
	{
		// GL device info
		auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
			= m_plot->GetImpl()->GetGlDescr();
		m_labelGlInfos[0]->setText(QString("GL Version: ") + strGlVer.c_str() + QString("."));
		m_labelGlInfos[1]->setText(QString("GL Shader Version: ") + strGlShaderVer.c_str() + QString("."));
		m_labelGlInfos[2]->setText(QString("GL Vendor: ") + strGlVendor.c_str() + QString("."));
		m_labelGlInfos[3]->setText(QString("GL Device: ") + strGlRenderer.c_str() + QString("."));


		if(!m_3dobjsReady)		// create 3d objects
		{
			m_arrow_pi = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.,0.,0.85,1.);
			m_arrow_pf = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.,0.5,0.,1.);
			m_arrow_M_Re = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.,0.,1.);
			m_arrow_M_Im = m_plot->GetImpl()->AddArrow(0.05, 1., 0.,0.,0.5,  0.85,0.25,0.,1.);

			m_plot->GetImpl()->SetObjectLabel(m_arrow_pi, "P_i");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_pf, "P_f");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Re, "Re{M_perp}");
			m_plot->GetImpl()->SetObjectLabel(m_arrow_M_Im, "Im{M_perp}");

			m_3dobjsReady = true;
			CalcPol();
		}
	}


	/**
	 * get the length of a vector
	 */
	t_real GetArrowLen(std::size_t objIdx) const
	{
		if(objIdx == m_arrow_pi)
		{
			return std::sqrt(std::pow(m_editPiX->text().toDouble(), 2.)
				+ std::pow(m_editPiY->text().toDouble(), 2.)
				+ std::pow(m_editPiZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_pf)
		{
			return std::sqrt(std::pow(m_editPfX->text().toDouble(), 2.)
				+ std::pow(m_editPfY->text().toDouble(), 2.)
				+ std::pow(m_editPfZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_M_Re)
		{
			return std::sqrt(std::pow(m_editMPerpReX->text().toDouble(), 2.)
				+ std::pow(m_editMPerpReY->text().toDouble(), 2.)
				+ std::pow(m_editMPerpReZ->text().toDouble(), 2.));
		}
		else if(objIdx == m_arrow_M_Im)
		{
			return std::sqrt(std::pow(m_editMPerpImX->text().toDouble(), 2.)
				+ std::pow(m_editMPerpImY->text().toDouble(), 2.)
				+ std::pow(m_editMPerpImZ->text().toDouble(), 2.));
		}

		return -1.;
	}


	/**
	 * called when the mouse hovers over an object
	 */
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
	{
		m_curPickedObj = -1;
		if(pos)
		{	// object selected?
			m_curPickedObj = long(objIdx);

			if(objIdx == m_arrow_pi) m_labelStatus->setText("P_i");
			else if(objIdx == m_arrow_pf) m_labelStatus->setText("P_f");
			else if(objIdx == m_arrow_M_Re) m_labelStatus->setText("Re{M_perp}");
			else if(objIdx == m_arrow_M_Im) m_labelStatus->setText("Im{M_perp}");
			else m_curPickedObj = -1;
		}

		if(m_curPickedObj >= 0)
		{
			setCursor(Qt::CrossCursor);
		}
		else
		{
			m_labelStatus->setText("");
			setCursor(Qt::ArrowCursor);
		}


		if(posSphere && m_mouseDown[0])
		{	// picker intersecting unit sphere and mouse dragged?

			t_vec3_gl posSph = *posSphere;

			if(m_curDraggedObj == m_arrow_pi)
			{
				m_editPiX->setText(QVariant(posSph[0]).toString());
				m_editPiY->setText(QVariant(posSph[1]).toString());
				m_editPiZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
			else if(m_curDraggedObj == m_arrow_M_Re)
			{
				m_editMPerpReX->setText(QVariant(posSph[0]).toString());
				m_editMPerpReY->setText(QVariant(posSph[1]).toString());
				m_editMPerpReZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
			else if(m_curDraggedObj == m_arrow_M_Im)
			{
				m_editMPerpImX->setText(QVariant(posSph[0]).toString());
				m_editMPerpImY->setText(QVariant(posSph[1]).toString());
				m_editMPerpImZ->setText(QVariant(posSph[2]).toString());
				CalcPol();
			}
		}
	}


	/**
	 * mouse button pressed
	 */
	void MouseDown(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = true;
		if(mid) m_mouseDown[1] = true;
		if(right) m_mouseDown[2] = true;

		if(m_mouseDown[0])
		{
			m_curDraggedObj = m_curPickedObj;
			auto lenVec = GetArrowLen(m_curDraggedObj);
			if(lenVec > 0.)
				m_plot->GetImpl()->SetPickerSphereRadius(lenVec);
		}
	}


	/**
	 * mouse button released
	 */
	void MouseUp(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = false;
		if(mid) m_mouseDown[1] = false;
		if(right) m_mouseDown[2] = false;

		if(!m_mouseDown[0])
			m_curDraggedObj = -1;
	}


public:
	PolDlg() = delete;

	/**
	 * create UI
	 */
	PolDlg(QWidget* pParent) : QDialog{pParent, Qt::Window}
	{
		setWindowTitle("Polarisation Vectors");
		setSizeGripEnabled(true);

		auto tabs = new QTabWidget(this);


		{	// plot panel
			auto plotpanel = new QWidget(this);

			auto labelN = new QLabel("Re{N}, Im{N}:", plotpanel);
			auto labelMPerpRe = new QLabel("Re{M_perp}:", plotpanel);
			auto labelMPerpIm = new QLabel("Im{M_perp}:", plotpanel);
			auto labelPi = new QLabel("P_i:", plotpanel);
			auto labelPf = new QLabel("P_f:", plotpanel);


			for(auto* label : {labelMPerpRe, labelMPerpIm, labelPi, labelPf})
				label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

			for(auto* editPf : {m_editPfX, m_editPfY, m_editPfZ})
				editPf->setReadOnly(true);

			m_labelStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			m_labelStatus->setFrameStyle(QFrame::Sunken | QFrame::Panel);
			m_labelStatus->setLineWidth(1);


			// connections
			for(auto* edit : {m_editNRe, m_editNIm,
				m_editMPerpReX, m_editMPerpReY, m_editMPerpReZ,
				m_editMPerpImX, m_editMPerpImY, m_editMPerpImZ,
				m_editPiX, m_editPiY, m_editPiZ,
				m_editPfX, m_editPfY, m_editPfZ})
				connect(edit, &QLineEdit::textEdited, this, &PolDlg::CalcPol);

			connect(m_plot.get(), &GlPlot::AfterGLInitialisation, this, &PolDlg::AfterGLInitialisation);
			connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &PolDlg::PickerIntersection);

			connect(m_plot.get(), &GlPlot::MouseDown, this, &PolDlg::MouseDown);
			connect(m_plot.get(), &GlPlot::MouseUp, this, &PolDlg::MouseUp);


			auto pGrid = new QGridLayout(plotpanel);
			pGrid->setSpacing(4);
			pGrid->setContentsMargins(4,4,4,4);

			pGrid->addWidget(m_plot.get(), 0,0,1,4);

			pGrid->addWidget(labelN, 1,0,1,1);
			pGrid->addWidget(labelMPerpRe, 2,0,1,1);
			pGrid->addWidget(labelMPerpIm, 3,0,1,1);
			pGrid->addWidget(labelPi, 4,0,1,1);
			pGrid->addWidget(labelPf, 5,0,1,1);

			pGrid->addWidget(m_editNRe, 1,1,1,1);
			pGrid->addWidget(m_editNIm, 1,2,1,1);

			pGrid->addWidget(m_editMPerpReX, 2,1,1,1);
			pGrid->addWidget(m_editMPerpReY, 2,2,1,1);
			pGrid->addWidget(m_editMPerpReZ, 2,3,1,1);
			pGrid->addWidget(m_editMPerpImX, 3,1,1,1);
			pGrid->addWidget(m_editMPerpImY, 3,2,1,1);
			pGrid->addWidget(m_editMPerpImZ, 3,3,1,1);

			pGrid->addWidget(m_editPiX, 4,1,1,1);
			pGrid->addWidget(m_editPiY, 4,2,1,1);
			pGrid->addWidget(m_editPiZ, 4,3,1,1);
			pGrid->addWidget(m_editPfX, 5,1,1,1);
			pGrid->addWidget(m_editPfY, 5,2,1,1);
			pGrid->addWidget(m_editPfZ, 5,3,1,1);

			pGrid->addWidget(m_labelStatus, 6,0,1,4);

			// restore last values
			if(m_sett.contains("n_re")) m_editNRe->setText(m_sett.value("n_re").toString());
			if(m_sett.contains("n_im")) m_editNIm->setText(m_sett.value("n_im").toString());
			if(m_sett.contains("mx_re")) m_editMPerpReX->setText(m_sett.value("mx_re").toString());
			if(m_sett.contains("my_re")) m_editMPerpReY->setText(m_sett.value("my_re").toString());
			if(m_sett.contains("mz_re")) m_editMPerpReZ->setText(m_sett.value("mz_re").toString());
			if(m_sett.contains("mx_im")) m_editMPerpImX->setText(m_sett.value("mx_im").toString());
			if(m_sett.contains("my_im")) m_editMPerpImY->setText(m_sett.value("my_im").toString());
			if(m_sett.contains("mz_im")) m_editMPerpImZ->setText(m_sett.value("mz_im").toString());
			if(m_sett.contains("pix")) m_editPiX->setText(m_sett.value("pix").toString());
			if(m_sett.contains("piy")) m_editPiY->setText(m_sett.value("piy").toString());
			if(m_sett.contains("piz")) m_editPiZ->setText(m_sett.value("piz").toString());


			tabs->addTab(plotpanel, "Calculation");
		}


		{	// info panel
			auto infopanel = new QWidget(this);
			auto pGrid = new QGridLayout(infopanel);
			pGrid->setSpacing(4);
			pGrid->setContentsMargins(4,4,4,4);

			auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
			auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
			auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

			for(int i=0; i<4; ++i)
			{
				m_labelGlInfos[i] = new QLabel("", infopanel);
				m_labelGlInfos[i]->setSizePolicy(QSizePolicy::Ignored, m_labelGlInfos[i]->sizePolicy().verticalPolicy());
			}

			std::string strBoost = BOOST_LIB_VERSION;
			algo::replace_all(strBoost, "_", ".");

			auto labelTitle = new QLabel("Polarisation Calculator", infopanel);
			auto fontTitle = labelTitle->font();
			fontTitle.setBold(true);
			labelTitle->setFont(fontTitle);
			labelTitle->setAlignment(Qt::AlignHCenter);

			auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
			labelAuthor->setAlignment(Qt::AlignHCenter);

			auto labelDate = new QLabel("November 2018.", infopanel);
			labelDate->setAlignment(Qt::AlignHCenter);

			int y = 0;
			pGrid->addWidget(labelTitle, y++,0, 1,1);
			pGrid->addWidget(labelAuthor, y++,0, 1,1);
			pGrid->addWidget(labelDate, y++,0, 1,1);
			pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Fixed), y++,0, 1,1);
			pGrid->addWidget(sep1, y++,0, 1,1);
			pGrid->addWidget(new QLabel(QString("Compiler: ") + QString(BOOST_COMPILER) + ".", infopanel), y++,0, 1,1);
			pGrid->addWidget(new QLabel(QString("C++ Library: ") + QString(BOOST_STDLIB) + ".", infopanel), y++,0, 1,1);
			pGrid->addWidget(new QLabel(QString("Build Date: ") + QString(__DATE__) + ", " + QString(__TIME__) + ".", infopanel), y++,0, 1,1);
			pGrid->addWidget(sep2, y++,0, 1,1);
			pGrid->addWidget(new QLabel(QString("Qt Version: ") + QString(QT_VERSION_STR) + ".", infopanel), y++,0, 1,1);
			pGrid->addWidget(new QLabel(QString("Boost Version: ") + strBoost.c_str() + ".", infopanel), y++,0, 1,1);
			pGrid->addWidget(sep3, y++,0, 1,1);
			for(int i=0; i<4; ++i)
				pGrid->addWidget(m_labelGlInfos[i], y++,0, 1,1);
			pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Expanding), y++,0, 1,1);

			tabs->addTab(infopanel, "Infos");
		}


		auto pmainGrid = new QGridLayout(this);
		pmainGrid->setSpacing(4);
		pmainGrid->setContentsMargins(4,4,4,4);
		pmainGrid->addWidget(tabs, 0,0, 1,1);


		// restory window size and position
		if(m_sett.contains("geo"))
			restoreGeometry(m_sett.value("geo").toByteArray());
		else
			resize(800, 600);

		// have scattering plane in horizontal plane
		m_plot->GetImpl()->SetCamBase(m::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-5,  0,0,0,1}),
			m::create<t_vec_gl>({1,0,0,0}), m::create<t_vec_gl>({0,0,1,0}));
		CalcPol();
	}


	/**
	 * calculate final polarisation vector
	 */
	void CalcPol()
	{
		// get values from line edits
		t_real NRe = t_real(m_editNRe->text().toDouble());
		t_real NIm = t_real(m_editNIm->text().toDouble());

		t_real MPerpReX = t_real(m_editMPerpReX->text().toDouble());
		t_real MPerpReY = t_real(m_editMPerpReY->text().toDouble());
		t_real MPerpReZ = t_real(m_editMPerpReZ->text().toDouble());
		t_real MPerpImX = t_real(m_editMPerpImX->text().toDouble());
		t_real MPerpImY = t_real(m_editMPerpImY->text().toDouble());
		t_real MPerpImZ = t_real(m_editMPerpImZ->text().toDouble());

		t_real PiX = t_real(m_editPiX->text().toDouble());
		t_real PiY = t_real(m_editPiY->text().toDouble());
		t_real PiZ = t_real(m_editPiZ->text().toDouble());

		const t_cplx N(NRe, NIm);
		const t_vec Mperp = m::create<t_vec>({
			t_cplx(MPerpReX,MPerpImX),
			t_cplx(MPerpReY,MPerpImY),
			t_cplx(MPerpReZ,MPerpImZ) });
		const t_vec Pi = m::create<t_vec>({PiX, PiY, PiZ});

		// calculate final polarisation vector and intensity
		auto [I, P_f] = m::blume_maleev_indir<t_mat, t_vec, t_cplx>(Pi, Mperp, N);
		//auto [I, P_f] = m::blume_maleev<t_vec, t_cplx>(Pi, Mperp, N);

		// set final polarisation
		m_editPfX->setText(std::to_string(P_f[0].real()).c_str());
		m_editPfY->setText(std::to_string(P_f[1].real()).c_str());
		m_editPfZ->setText(std::to_string(P_f[2].real()).c_str());


		// update 3d objects
		if(m_3dobjsReady)
		{
			// P_i
			t_mat_gl matPi = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(PiX), t_real_gl(PiY), t_real_gl(PiZ)}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_pi, matPi);

			// P_f
			t_mat_gl matPf = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(P_f[0].real()), t_real_gl(P_f[1].real()), t_real_gl(P_f[2].real())}), 	// to
				1., 								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_pf, matPf);

			// Re(M)
			const t_real_gl lenReM = t_real_gl(std::sqrt(MPerpReX*MPerpReX + MPerpReY*MPerpReY + MPerpReZ*MPerpReZ));
			t_mat_gl matMRe = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpReX), t_real_gl(MPerpReY), t_real_gl(MPerpReZ)}), 	// to
				lenReM,								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Re, matMRe);
			m_plot->GetImpl()->SetObjectVisible(m_arrow_M_Re, !m::equals(lenReM, t_real_gl(0)));

			// Im(M)
			const t_real_gl lenImM = t_real_gl(std::sqrt(MPerpImX*MPerpImX + MPerpImY*MPerpImY + MPerpImZ*MPerpImZ));
			t_mat_gl matMIm = GlPlot_impl::GetArrowMatrix(
				m::create<t_vec_gl>({t_real_gl(MPerpImX), t_real_gl(MPerpImY), t_real_gl(MPerpImZ)}), 	// to
				lenImM,								// scale
				m::create<t_vec_gl>({0,0,0.5}),		// translate 
				m::create<t_vec_gl>({0,0,1}));		// from
			m_plot->GetImpl()->SetObjectMatrix(m_arrow_M_Im, matMIm);
			m_plot->GetImpl()->SetObjectVisible(m_arrow_M_Im, !m::equals(lenImM, t_real_gl(0)));

			m_plot->update();
		}
	}
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
#ifndef BUILD_LIB	// build application


int main(int argc, char** argv)
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();
	auto app = std::make_unique<QApplication>(argc, argv);

	auto dlg = std::make_unique<PolDlg>(nullptr);
	dlg->show();

	return app->exec();
}


#else	// build library


#include <boost/dll/alias.hpp>


/**
 * initialise plugin
 */
bool init()
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	return true;
}


/**
 * plugin descriptor
 * type, title, description
 */
std::tuple<std::string, std::string, std::string> descr()
{
	return std::make_tuple("dlg", "Polarisation Vectors", "Calculates polarisation vectors.");
}


/**
 * create the plugin main dialog
 */
std::shared_ptr<QDialog> create(QWidget *pParent)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	return std::make_shared<PolDlg>(pParent);
}


BOOST_DLL_ALIAS(init, tl_init);
BOOST_DLL_ALIAS(descr, tl_descr);
BOOST_DLL_ALIAS(create, tl_create);


#endif
// ----------------------------------------------------------------------------
