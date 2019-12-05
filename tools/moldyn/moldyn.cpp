/**
 * atom dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 */

#include "moldyn.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
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


	QWidget *pMainPanel = new QWidget();
	auto pMainGrid = new QGridLayout(pMainPanel);
	pMainGrid->setSpacing(2);
	pMainGrid->setContentsMargins(4,4,4,4);
	this->setCentralWidget(pMainPanel);


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		//auto acSave = new QAction("Save...", menuFile);
		auto acExit = new QAction("Exit", menuFile);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		//menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acExit);

		connect(acNew, &QAction::triggered, this, &MolDynDlg::New);
		connect(acLoad, &QAction::triggered, this, &MolDynDlg::Load);
		//connect(acSave, &QAction::triggered, this, &MolDynDlg::Save);
		connect(acExit, &QAction::triggered, this, &QDialog::close);

		m_menu->addMenu(menuFile);
		this->setMenuBar(m_menu);
	}


	// plot widget
	{
		m_plot = new GlPlot(this);
		m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_plot->GetImpl()->EnablePicker(1);
		m_plot->GetImpl()->SetLight(0, m::create<t_vec3_gl>({ 5, 5, 5 }));
		m_plot->GetImpl()->SetLight(1, m::create<t_vec3_gl>({ -5, -5, -5 }));
		m_plot->GetImpl()->SetCoordMax(1.);
		m_plot->GetImpl()->SetCamBase(m::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-1.5,  0,0,0,1}),
			m::create<t_vec_gl>({1,0,0,0}), m::create<t_vec_gl>({0,0,1,0}));

		connect(m_plot, &GlPlot::AfterGLInitialisation, this, &MolDynDlg::AfterGLInitialisation);
		connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &MolDynDlg::PickerIntersection);
		connect(m_plot, &GlPlot::MouseDown, this, &MolDynDlg::PlotMouseDown);
		connect(m_plot, &GlPlot::MouseUp, this, &MolDynDlg::PlotMouseUp);

		//this->setCentralWidget(m_plot);
		pMainGrid->addWidget(m_plot, 0,0,1,1);
	}


	// controls
	{
		m_slider = new QSlider(Qt::Horizontal, this);
		m_slider->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Minimum});
		m_slider->setMinimum(0);
		m_slider->setTracking(1);

		connect(m_slider, &QSlider::valueChanged, this, &MolDynDlg::SliderValueChanged);

		pMainGrid->addWidget(m_slider, 1,0,1,1);
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
 * add an atom
 */
std::size_t MolDynDlg::Add3DItem(const t_vec& vec, const t_vec& col, t_real scale, const std::string& label)
{
	auto obj = m_plot->GetImpl()->AddLinkedObject(m_sphere, 0,0,0, col[0],col[1],col[2],1);
	Change3DItem(obj, &vec, &col, &scale, &label);
	return obj;
}


/**
 * change an atom
 */
void MolDynDlg::Change3DItem(std::size_t obj, const t_vec *vec, const t_vec *col, const t_real *scale, const std::string *label)
{
	t_mat_gl mat = m::hom_translation<t_mat_gl>((*vec)[0], (*vec)[1], (*vec)[2]);
	if(scale) mat *= m::hom_scaling<t_mat_gl>(*scale, *scale, *scale);

	m_plot->GetImpl()->SetObjectMatrix(obj, mat);
	if(col) m_plot->GetImpl()->SetObjectCol(obj, (*col)[0], (*col)[1], (*col)[2], 1);
	if(label) m_plot->GetImpl()->SetObjectLabel(obj, *label);
	if(label) m_plot->GetImpl()->SetObjectDataString(obj, *label);
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
void MolDynDlg::New()
{
	m_mol.Clear();
	m_sphereHandles.clear();
	// TODO: clear 3d objects
}


void MolDynDlg::Load()
{
	if(!m_plot) return;

	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "Molecular Dynamics File (*)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());

		if(!m_mol.LoadFile(filename.toStdString(), m_frameskip))
		{
			QMessageBox::critical(this, "Molecular Dynamics", "Error loading file.");
			return;
		}

		m_slider->setMaximum(m_mol.GetFrameCount());


		// atom colors
		std::vector<t_vec> cols =
		{
			m::create<t_vec>({1, 0, 0}),
			m::create<t_vec>({0, 0, 1}),
			m::create<t_vec>({0, 0.5, 0}),
			m::create<t_vec>({0, 0.5, 0.5}),
			m::create<t_vec>({0.5, 0.5, 0}),
			m::create<t_vec>({0, 0, 0}),
		};

		// add atoms to 3d view
		if(m_mol.GetFrameCount())
		{
			const auto& frame = m_mol.GetFrame(0);
			m_sphereHandles.reserve(frame.GetNumAtoms());

			for(std::size_t atomidx=0; atomidx<frame.GetNumAtoms(); ++atomidx)
			{
				const auto& coords = frame.GetCoords(atomidx);
				for(const t_vec& vec : coords)
				{
					std::size_t handle = Add3DItem(vec, cols[atomidx % cols.size()], m_atomscale, m_mol.GetAtomName(atomidx));
					m_sphereHandles.push_back(handle);
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Molecular Dynamics", ex.what());
	}

	m_plot->update();
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
	if(!m_plot) return;

	if(pos)
		m_curPickedObj = long(objIdx);
	else
		m_curPickedObj = -1;


	if(m_curPickedObj > 0)
	{
		const std::string& label = m_plot->GetImpl()->GetObjectDataString(m_curPickedObj);
		SetStatusMsg(label);
	}
	else
	{
		SetStatusMsg("");
	}
}



/**
 * set status label text in 3d dialog
 */
void MolDynDlg::SetStatusMsg(const std::string& msg)
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



void MolDynDlg::SliderValueChanged(int val)
{
	if(!m_plot) return;

	if(val < 0 || val >= m_mol.GetFrameCount())
		return;


	// update atom position with selected frame
	const auto& frame = m_mol.GetFrame(val);

	std::size_t counter = 0;
	for(std::size_t atomidx=0; atomidx<frame.GetNumAtoms(); ++atomidx)
	{
		const auto& coords = frame.GetCoords(atomidx);
		for(const t_vec& vec : coords)
		{
			std::size_t obj = m_sphereHandles[counter];
			Change3DItem(obj, &vec, nullptr, &m_atomscale);

			++counter;
		}
	}

	m_plot->update();
}



// ----------------------------------------------------------------------------
void MolDynDlg::AfterGLInitialisation()
{
	if(!m_plot) return;

	// reference sphere for linked objects
	m_sphere = m_plot->GetImpl()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_plot->GetImpl()->SetObjectVisible(m_sphere, false);

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
