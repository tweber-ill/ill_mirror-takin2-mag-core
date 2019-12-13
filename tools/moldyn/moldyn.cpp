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
#include <QtWidgets/QProgressDialog>

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

		// File
		auto menuFile = new QMenu("File", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSaveAs = new QAction("Save As...", menuFile);
		auto acExit = new QAction("Exit", menuFile);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSaveAs);
		menuFile->addSeparator();
		menuFile->addAction(acExit);

		connect(acNew, &QAction::triggered, this, &MolDynDlg::New);
		connect(acLoad, &QAction::triggered, this, &MolDynDlg::Load);
		connect(acSaveAs, &QAction::triggered, this, &MolDynDlg::SaveAs);
		connect(acExit, &QAction::triggered, this, &QDialog::close);


		// Edit
		auto menuEdit = new QMenu("Edit", m_menu);
		auto acSelectNone = new QAction("Select None", menuEdit);

		menuEdit->addAction(acSelectNone);

		connect(acSelectNone, &QAction::triggered, this, &MolDynDlg::SelectNone);


		// Calculate
		auto menuCalc = new QMenu("Calculate", m_menu);
		auto acCalcDist = new QAction("Distance Between Selected Atoms", menuEdit);

		menuCalc->addAction(acCalcDist);

		connect(acCalcDist, &QAction::triggered, this, &MolDynDlg::CalculateDistanceBetweenAtoms);


		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuEdit);
		m_menu->addMenu(menuCalc);
		this->setMenuBar(m_menu);
	}


	// context menus
	{
		m_atomContextMenu = new QMenu(this);
		m_atomContextMenu->setTitle("Atoms");
		m_atomContextMenu->addAction("Delete Atom", this, &MolDynDlg::DeleteAtomUnderCursor);
		m_atomContextMenu->addAction("Delete All Atoms Of Selected Type", this, &MolDynDlg::DeleteAllAtomsOfSameType);
		m_atomContextMenu->addAction("Only Keep Atoms Of Selected Type", this, &MolDynDlg::KeepAtomsOfSameType);
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
		connect(m_plot, &GlPlot::MouseClick, this, &MolDynDlg::PlotMouseClick);

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
/**
 * calculate the distance between selected atoms
 */
void MolDynDlg::CalculateDistanceBetweenAtoms()
{
	std::vector<std::tuple<std::size_t, std::size_t>> objs;

	for(const auto& obj : m_sphereHandles)
	{
		// continue if object isn't selected
		if(!m_plot->GetImpl()->GetObjectHighlight(obj))
			continue;

		// get indices for selected atoms
		const auto [bOk, atomTypeIdx, atomSubTypeIdx, sphereIdx] = GetAtomIndexFromHandle(obj);
		if(!bOk)
		{
			QMessageBox::critical(this, "Molecular Dynamics", "Atom handle not found, data is corrupted.");
			return;
		}

		objs.push_back(std::make_tuple(atomTypeIdx, atomSubTypeIdx));
	}

	if(objs.size() <= 1)
	{
		QMessageBox::critical(this, "Molecular Dynamics", "At least two atoms have to be selected.");
		return;
	}


	// get coordinates of first atom
	auto [firstObjTypeIdx, firstObjSubTypeIdx] = objs[0];
	auto firstObjCoords = m_mol.GetAtomCoords(firstObjTypeIdx, firstObjSubTypeIdx);

	std::vector<t_vec> firstObjCoordsCryst;
	firstObjCoordsCryst.reserve(m_mol.GetFrameCount());

	for(std::size_t frameidx=0; frameidx<m_mol.GetFrameCount(); ++frameidx)
		firstObjCoordsCryst.push_back(m_crystA * firstObjCoords[frameidx]);


	// get distances to other selected atoms
	for(std::size_t objIdx=1; objIdx<objs.size(); ++objIdx)
	{
		auto [objTypeIdx, objSubTypeIdx] = objs[objIdx];
		const auto objCoords = m_mol.GetAtomCoords(objTypeIdx, objSubTypeIdx);
		
		for(std::size_t frameidx=0; frameidx<m_mol.GetFrameCount(); ++frameidx)
		{
			t_real dist = m::get_dist_uc(m_crystA, firstObjCoordsCryst[frameidx], objCoords[frameidx]);

			// TODO: plot/save/...
			std::cout << dist << std::endl;
		}
	}
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
		if(filename == "" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());

		New();


		std::shared_ptr<QProgressDialog> dlgProgress = std::make_shared<QProgressDialog>(
			"Loading \"" + QFileInfo(filename).fileName() + "\"...", "Cancel", 0, 1000, this);
		bool bCancelled = 0;
		auto progressHandler = [dlgProgress, &bCancelled](t_real percentage) -> bool
		{
			dlgProgress->setValue(int(percentage*10));
			bCancelled = dlgProgress->wasCanceled();
			return !bCancelled;
		};
		m_mol.SubscribeToLoadProgress(progressHandler);
		dlgProgress->setWindowModality(Qt::WindowModal);


		if(!m_mol.LoadFile(filename.toStdString(), filedlg->GetFrameSkip()))
		{
			// only show error if not explicitely cancelled
			if(!bCancelled)
				QMessageBox::critical(this, "Molecular Dynamics", "Error loading file.");
			return;
		}


		m_mol.UnsubscribeFromLoadProgress(&progressHandler);
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


void MolDynDlg::SaveAs()
{
	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "Molecular Dynamics File (*)");
		if(filename == "")
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());


		std::shared_ptr<QProgressDialog> dlgProgress = std::make_shared<QProgressDialog>(
			"Saving \"" + QFileInfo(filename).fileName() + "\"...", "Cancel", 0, 1000, this);
		bool bCancelled = 0;
		auto progressHandler = [dlgProgress, &bCancelled](t_real percentage) -> bool
		{
			dlgProgress->setValue(int(percentage*10));
			bCancelled = dlgProgress->wasCanceled();
			return !bCancelled;
		};
		m_mol.SubscribeToSaveProgress(progressHandler);
		dlgProgress->setWindowModality(Qt::WindowModal);


		if(!m_mol.SaveFile(filename.toStdString()))
		{
			QMessageBox::critical(this, "Molecular Dynamics", "Error saving file.");
		}


		m_mol.UnsubscribeFromSaveProgress(&progressHandler);
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Molecular Dynamics", ex.what());
	}
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
		m_plot->GetImpl()->SetObjectHighlight(m_curPickedObj, !m_plot->GetImpl()->GetObjectHighlight(m_curPickedObj));
		m_plot->update();
	}
}


/**
 * mouse button released
 */
void MolDynDlg::PlotMouseUp(bool left, bool mid, bool right)
{
}


/**
 * mouse button clicked
 */
void MolDynDlg::PlotMouseClick(bool left, bool mid, bool right)
{
	// show context menu
	if(right && m_curPickedObj > 0)
	{
		QString atomLabel = m_plot->GetImpl()->GetObjectDataString(m_curPickedObj).c_str();
		m_atomContextMenu->actions()[0]->setText("Delete This \"" + atomLabel + "\" Atom");
		m_atomContextMenu->actions()[1]->setText("Delete All \"" + atomLabel + "\" Atoms");
		m_atomContextMenu->actions()[2]->setText("Delete All But \"" + atomLabel + "\" Atoms");

		auto ptGlob = QCursor::pos();
		ptGlob.setY(ptGlob.y() + 8);
		m_atomContextMenu->popup(ptGlob);
	}
}


// ----------------------------------------------------------------------------


/**
 * unselect all atoms
 */
void MolDynDlg::SelectNone()
{
	if(!m_plot) return;

	for(auto handle : m_sphereHandles)
		m_plot->GetImpl()->SetObjectHighlight(handle, 0);

	m_plot->update();
}


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


/**
 * get the index of the atom in the m_mol data structure
 * from the handle of the displayed 3d object
 */
std::tuple<bool, std::size_t, std::size_t, std::size_t> 
MolDynDlg::GetAtomIndexFromHandle(std::size_t handle) const
{
	// find handle in sphere handle vector
	auto iter = std::find(m_sphereHandles.begin(), m_sphereHandles.end(), handle);
	if(iter == m_sphereHandles.end())
		return std::make_tuple(0, 0, 0, 0);

	std::size_t sphereIdx = iter - m_sphereHandles.begin();

	std::size_t atomCountsSoFar = 0;
	std::size_t atomTypeIdx = 0;
	for(atomTypeIdx=0; atomTypeIdx<m_mol.GetAtomCount(); ++atomTypeIdx)
	{
		std::size_t numAtoms = m_mol.GetAtomNum(atomTypeIdx);
		if(atomCountsSoFar + numAtoms > sphereIdx)
			break;

		atomCountsSoFar += numAtoms;
	}

	std::size_t atomSubTypeIdx = sphereIdx-atomCountsSoFar;

	return std::make_tuple(1, atomTypeIdx, atomSubTypeIdx, sphereIdx);
}


/**
 * delete one atom
 */
void MolDynDlg::DeleteAtomUnderCursor()
{
	// nothing under cursor
	if(m_curPickedObj <= 0)
		return;

	// atom type to be deleted
	const std::string& atomLabel = m_plot->GetImpl()->GetObjectDataString(m_curPickedObj);

	const auto [bOk, atomTypeIdx, atomSubTypeIdx, sphereIdx] = GetAtomIndexFromHandle(m_curPickedObj);
	if(!bOk)
	{
		QMessageBox::critical(this, "Molecular Dynamics", "Atom handle not found, data is corrupted.");
		return;
	}

	if(m_mol.GetAtomName(atomTypeIdx) != atomLabel)
	{
		QMessageBox::critical(this, "Molecular Dynamics", "Mismatch in atom type, data is corrupted.");
		return;
	}

	// remove 3d objects
	m_plot->GetImpl()->RemoveObject(m_sphereHandles[sphereIdx]);
	m_sphereHandles.erase(m_sphereHandles.begin()+sphereIdx);

	// remove atom
	m_mol.RemoveAtom(atomTypeIdx, atomSubTypeIdx);

	SetStatusMsg("1 atom removed.");
	m_plot->update();
}


/**
 * delete all atoms of the type under the cursor
 */
void MolDynDlg::DeleteAllAtomsOfSameType()
{
	// nothing under cursor
	if(m_curPickedObj <= 0)
		return;

	// atom type to be deleted
	const std::string& atomLabel = m_plot->GetImpl()->GetObjectDataString(m_curPickedObj);

	std::size_t startIdx = 0;
	std::size_t totalRemoved = 0;
	for(std::size_t atomIdx=0; atomIdx<m_mol.GetAtomCount();)
	{
		std::size_t numAtoms = m_mol.GetAtomNum(atomIdx);

		if(m_mol.GetAtomName(atomIdx) == atomLabel)
		{
			// remove 3d objects
			for(std::size_t sphereIdx=startIdx; sphereIdx<startIdx+numAtoms; ++sphereIdx)
				m_plot->GetImpl()->RemoveObject(m_sphereHandles[sphereIdx]);
			m_sphereHandles.erase(m_sphereHandles.begin()+startIdx, m_sphereHandles.begin()+startIdx+numAtoms);

			// remove atoms
			m_mol.RemoveAtoms(atomIdx);

			totalRemoved += numAtoms;
		}
		else
		{
			startIdx += numAtoms;
			++atomIdx;
		}
	}

	SetStatusMsg(std::to_string(totalRemoved) + " atoms removed.");
	m_plot->update();
}


/**
 * delete all atoms NOT of the type under the cursor
 */
void MolDynDlg::KeepAtomsOfSameType()
{
	// nothing under cursor
	if(m_curPickedObj <= 0)
		return;

	// atom type to be deleted
	const std::string& atomLabel = m_plot->GetImpl()->GetObjectDataString(m_curPickedObj);

	std::size_t startIdx = 0;
	std::size_t totalRemoved = 0;
	for(std::size_t atomIdx=0; atomIdx<m_mol.GetAtomCount();)
	{
		std::size_t numAtoms = m_mol.GetAtomNum(atomIdx);

		if(m_mol.GetAtomName(atomIdx) != atomLabel)
		{
			// remove 3d objects
			for(std::size_t sphereIdx=startIdx; sphereIdx<startIdx+numAtoms; ++sphereIdx)
				m_plot->GetImpl()->RemoveObject(m_sphereHandles[sphereIdx]);
			m_sphereHandles.erase(m_sphereHandles.begin()+startIdx, m_sphereHandles.begin()+startIdx+numAtoms);

			// remove atoms
			m_mol.RemoveAtoms(atomIdx);

			totalRemoved += numAtoms;
		}
		else
		{
			startIdx += numAtoms;
			++atomIdx;
		}
	}

	SetStatusMsg(std::to_string(totalRemoved) + " atoms removed.");
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
