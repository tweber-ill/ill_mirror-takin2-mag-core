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
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <tuple>
#include <memory>

#include "libs/algos.h"
#include "libs/helper.h"

using namespace m_ops;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;



// ----------------------------------------------------------------------------
/**
 * File dialog with options
 */

class MolDynFileDlg : public QFileDialog
{
	public:
		MolDynFileDlg(QWidget *parent, const QString& title, const QString& dir, const QString& filter)
			: QFileDialog(parent, title, dir, filter)
		{
			// options panel with frame skip
			QLabel *labelFrameSkip = new QLabel("Frame Skip: ", this);
			m_spinFrameSkip = new QSpinBox(this);
			m_spinFrameSkip->setValue(10);
			m_spinFrameSkip->setSingleStep(1);
			m_spinFrameSkip->setRange(0, 9999999);

			labelFrameSkip->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
			m_spinFrameSkip->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

			QWidget *pPanel = new QWidget();
			auto pPanelGrid = new QGridLayout(pPanel);
			pPanelGrid->setSpacing(2);
			pPanelGrid->setContentsMargins(4,4,4,4);

			pPanelGrid->addWidget(labelFrameSkip, 0,0,1,1);
			pPanelGrid->addWidget(m_spinFrameSkip, 0,1,1,1);

			// add the options panel to the layout
			setOptions(QFileDialog::DontUseNativeDialog);
			QGridLayout *pGrid = reinterpret_cast<QGridLayout*>(layout());
			if(pGrid)
				pGrid->addWidget(pPanel, pGrid->rowCount(), 0, 1, pGrid->columnCount());
		}


		int GetFrameSkip() const
		{
			return m_spinFrameSkip->value();
		}


	private:
		QSpinBox *m_spinFrameSkip = nullptr;
};



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
		pMainGrid->addWidget(m_plot, 0,0,1,9);
	}


	// controls
	{
		auto labCoordSys = new QLabel("Coordinates:", this);
		auto labFrames = new QLabel("Frames:", this);
		auto labScale = new QLabel("Scale:", this);
		labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
		labFrames->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
		labScale->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

		auto comboCoordSys = new QComboBox(this);
		comboCoordSys->addItem("Fractional Units (rlu)");
		comboCoordSys->addItem("Lab Units (A)");
		comboCoordSys->setFocusPolicy(Qt::StrongFocus);

		m_spinScale = new QDoubleSpinBox(this);
		m_spinScale->setDecimals(4);
		m_spinScale->setRange(1e-4, 1e4);
		m_spinScale->setSingleStep(0.1);
		m_spinScale->setValue(0.4);
		m_spinScale->setFocusPolicy(Qt::StrongFocus);

		m_slider = new QSlider(Qt::Horizontal, this);
		m_slider->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Minimum});
		m_slider->setMinimum(0);
		m_slider->setSingleStep(1);
		m_slider->setPageStep(10);
		m_slider->setTracking(1);
		m_slider->setFocusPolicy(Qt::StrongFocus);

		connect(m_slider, &QSlider::valueChanged, this, &MolDynDlg::SliderValueChanged);
		connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
		{
			if(this->m_plot)
				this->m_plot->GetImpl()->SetCoordSys(val);
		});
		connect(m_spinScale, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double val)
		{
			if(!this->m_plot) return;

			// hack to trigger update
			SliderValueChanged(m_slider->value());
		});

		pMainGrid->addWidget(labCoordSys, 1,0,1,1);
		pMainGrid->addWidget(comboCoordSys, 1,1,1,1);
		pMainGrid->addWidget(labScale, 1,2,1,1);
		pMainGrid->addWidget(m_spinScale, 1,3,1,1);
		pMainGrid->addWidget(labFrames, 1,4,1,1);
		pMainGrid->addWidget(m_slider, 1,5,1,4);
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
	if(vec)
	{
		t_mat_gl mat = m::hom_translation<t_mat_gl>((*vec)[0], (*vec)[1], (*vec)[2]);
		if(scale) mat *= m::hom_scaling<t_mat_gl>(*scale, *scale, *scale);
		m_plot->GetImpl()->SetObjectMatrix(obj, mat);
	}

	if(col) m_plot->GetImpl()->SetObjectCol(obj, (*col)[0], (*col)[1], (*col)[2], 1);
	if(label) m_plot->GetImpl()->SetObjectLabel(obj, *label);
	if(label) m_plot->GetImpl()->SetObjectDataString(obj, *label);
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
void MolDynDlg::New()
{
	m_mol.Clear();

	for(const auto& obj : m_sphereHandles)
		m_plot->GetImpl()->RemoveObject(obj);

	m_sphereHandles.clear();
	m_slider->setValue(0);

	m_plot->update();
}


void MolDynDlg::Load()
{
	if(!m_plot) return;

	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		auto filedlg = std::make_shared<MolDynFileDlg>(this, "Load File", dirLast, "Molecular Dynamics File (*)");
		if(!filedlg->exec())
			return;
		auto files = filedlg->selectedFiles();
		if(!files.size())
			return;
		
		QString filename = files[0];
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());

		New();
		if(!m_mol.LoadFile(filename.toStdString(), filedlg->GetFrameSkip()))
		{
			QMessageBox::critical(this, "Molecular Dynamics", "Error loading file.");
			return;
		}

		m_slider->setMaximum(m_mol.GetFrameCount() - 1);


		// crystal A and B matrices
		const t_vec& _a = m_mol.GetBaseA();
		const t_vec& _b = m_mol.GetBaseB();
		const t_vec& _c = m_mol.GetBaseC();

		m_crystA = m::create<t_mat>({
			_a[0],	_b[0],	_c[0],
			_a[1],	_b[1],	_c[1],
			_a[2], 	_b[2],	_c[2] });

		bool ok = true;
		std::tie(m_crystB, ok) = m::inv(m_crystA);
		if(!ok)
		{
			m_crystB = m::unit<t_mat>();
			QMessageBox::critical(this, "Molecular Dynamics", "Error: Cannot invert A matrix.");
		}

		m_crystB /= t_real_gl(2)*m::pi<t_real_gl>;
		t_mat_gl matA{m_crystA};
		m_plot->GetImpl()->SetBTrafo(m_crystB, &matA);

		std::cout << "A matrix: " << m_crystA << ", \n"
			<< "B matrix: " << m_crystB << "." << std::endl;


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
					t_real atomscale = m_spinScale->value();
					std::size_t handle = Add3DItem(vec, cols[atomidx % cols.size()], atomscale, m_mol.GetAtomName(atomidx));
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
	if(!m_plot) return;

	if(left && m_curPickedObj > 0)
	{
		m_plot->GetImpl()->ToggleObjectHighlight(m_curPickedObj);
		m_plot->update();
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
	t_real atomscale = m_spinScale->value();

	std::size_t counter = 0;
	for(std::size_t atomidx=0; atomidx<frame.GetNumAtoms(); ++atomidx)
	{
		const auto& coords = frame.GetCoords(atomidx);
		for(const t_vec& vec : coords)
		{
			std::size_t obj = m_sphereHandles[counter];
			Change3DItem(obj, &vec, nullptr, &atomscale);

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

	// B matrix
	m_plot->GetImpl()->SetBTrafo(m_crystB);

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


void MolDynDlg::keyPressEvent(QKeyEvent *evt)
{
	if(evt->key()==Qt::Key_Left || evt->key()==Qt::Key_Down)
		m_slider->setValue(m_slider->value() - m_slider->singleStep());
	else if(evt->key()==Qt::Key_Right || evt->key()==Qt::Key_Up)
		m_slider->setValue(m_slider->value() + m_slider->singleStep());
	else if(evt->key()==Qt::Key_PageUp)
		m_slider->setValue(m_slider->value() + m_slider->pageStep());
	else if(evt->key()==Qt::Key_PageDown)
		m_slider->setValue(m_slider->value() - m_slider->pageStep());
	else if(evt->key()==Qt::Key_Home)
		m_slider->setValue(m_slider->minimum());
	else if(evt->key()==Qt::Key_End)
		m_slider->setValue(m_slider->maximum());

	QMainWindow::keyPressEvent(evt);
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
