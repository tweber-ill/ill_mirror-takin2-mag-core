/**
 * atom dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 */

#include "moldyn.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <tuple>

#include "libs/algos.h"
#include "libs/helper.h"

using namespace m_ops;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;


// ----------------------------------------------------------------------------
MolDynDlg::MolDynDlg(QWidget* pParent) : QMainWindow{pParent},
	m_sett{new QSettings{"tobis_stuff", "moldyn"}}
{
	setWindowTitle("Molecular Dynamics");
	this->setObjectName("moldyn");

	m_status = new QStatusBar(this);
	this->setStatusBar(m_status);


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acExit = new QAction("Exit", menuFile);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acExit);

		connect(acNew, &QAction::triggered, this, &MolDynDlg::New);
		connect(acLoad, &QAction::triggered, this, &MolDynDlg::Load);
		connect(acSave, &QAction::triggered, this, &MolDynDlg::Save);
		connect(acExit, &QAction::triggered, this, &QDialog::close);

		m_menu->addMenu(menuFile);
		this->setMenuBar(m_menu);
	}


	// plot widget
	{
		m_plot = new GlPlot(this);
		m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_plot->GetImpl()->SetLight(0, m::create<t_vec3_gl>({ 5, 5, 5 }));
		m_plot->GetImpl()->SetLight(1, m::create<t_vec3_gl>({ -5, -5, -5 }));
		m_plot->GetImpl()->SetCoordMax(1.);
		m_plot->GetImpl()->SetCamBase(m::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-1.5,  0,0,0,1}),
			m::create<t_vec_gl>({1,0,0,0}), m::create<t_vec_gl>({0,0,1,0}));

		connect(m_plot, &GlPlot::AfterGLInitialisation, this, &MolDynDlg::AfterGLInitialisation);
		connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &MolDynDlg::PickerIntersection);
		connect(m_plot, &GlPlot::MouseDown, this, &MolDynDlg::PlotMouseDown);
		connect(m_plot, &GlPlot::MouseUp, this, &MolDynDlg::PlotMouseUp);

		this->setCentralWidget(m_plot);
	}


	// restore window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);

	m_ignoreChanges = 0;
}



// ----------------------------------------------------------------------------
/**
 * add 3d object
 */
void MolDynDlg::Add3DItem(int row)
{
	if(!m_plot) return;

	// add all items
	if(row < 0)
	{
		//for(int row=0; row<m_nuclei->rowCount(); ++row)
		//	Add3DItem(row);
		return;
	}

	qreal r=1, g=1, b=1;
	QColor col;
	col.getRgbF(&r, &g, &b);

	//auto obj = m_plot->GetImpl()->AddLinkedObject(m_sphere, 0,0,0, r,g,b,1);
	//auto obj = m_plot->GetImpl()->AddSphere(0.05, 0,0,0, r,g,b,1);
	//m_plot->GetImpl()->SetObjectMatrix(obj, m::hom_translation<t_mat_gl>(posx, posy, posz)*m::hom_scaling<t_mat_gl>(scale,scale,scale));
	//m_plot->GetImpl()->SetObjectLabel(obj, itemName->text().toStdString());
	//m_plot->update();
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
void MolDynDlg::New()
{
}


void MolDynDlg::Load()
{
}


void MolDynDlg::Save()
{
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * mouse hovers over 3d object
 */
void MolDynDlg::PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
{
	if(pos)
		m_curPickedObj = long(objIdx);
	else
		m_curPickedObj = -1;


	if(m_curPickedObj > 0)
	{
	}
	else
	{
		Set3DStatusMsg("");
	}
}



/**
 * set status label text in 3d dialog
 */
void MolDynDlg::Set3DStatusMsg(const std::string& msg)
{
	if(!m_status) return;

	m_status->showMessage(msg.c_str());
}



/**
 * mouse button pressed
 */
void MolDynDlg::PlotMouseDown(bool left, bool mid, bool right)
{
	if(left && m_curPickedObj > 0)
	{
	}
}


/**
 * mouse button released
 */
void MolDynDlg::PlotMouseUp(bool left, bool mid, bool right)
{
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
void MolDynDlg::AfterGLInitialisation()
{
	if(!m_plot) return;

	// reference sphere for linked objects
	m_sphere = m_plot->GetImpl()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_plot->GetImpl()->SetObjectVisible(m_sphere, false);

	// add all 3d objects
	Add3DItem(-1);

	// GL device info
	auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
		= m_plot->GetImpl()->GetGlDescr();
	std::cout << "GL Version: " << strGlVer << ", Shader Version: " << strGlShaderVer << "." << std::endl;
	std::cout << "GL Device: " << strGlRenderer << ", " << strGlVendor << "." << std::endl;
}


void MolDynDlg::closeEvent(QCloseEvent *evt)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
	}
}
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<MolDynDlg>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------
