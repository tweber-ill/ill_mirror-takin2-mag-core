/**
 * import magnetic structure from a table
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2023
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "table_import.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>

#include "tlibs2/libs/str.h"



TableImportDlg::TableImportDlg(QWidget* parent, QSettings* sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Table Importer");
	setSizeGripEnabled(true);
	setFont(parent->font());

	// gui elements
	// --------------------------------------------------------------------------------
	QLabel *labelAtomIdx = new QLabel("Column Indices in Atom Positions Table:", this);
	m_spinAtomName = new QSpinBox(this);
	m_spinAtomX = new QSpinBox(this);
	m_spinAtomY = new QSpinBox(this);
	m_spinAtomZ = new QSpinBox(this);
	m_spinAtomSX = new QSpinBox(this);
	m_spinAtomSY = new QSpinBox(this);
	m_spinAtomSZ = new QSpinBox(this);
	m_spinAtomSMag = new QSpinBox(this);

	m_spinAtomName->setPrefix("name = ");
	m_spinAtomX->setPrefix("x = ");
	m_spinAtomY->setPrefix("y = ");
	m_spinAtomZ->setPrefix("z = ");
	m_spinAtomSX->setPrefix("Sx = ");
	m_spinAtomSY->setPrefix("Sy = ");
	m_spinAtomSZ->setPrefix("Sz = ");
	m_spinAtomSMag->setPrefix("|S| = ");

	m_spinAtomName->setValue(0);
	m_spinAtomX->setValue(1);
	m_spinAtomY->setValue(2);
	m_spinAtomZ->setValue(3);
	m_spinAtomSX->setValue(4);
	m_spinAtomSY->setValue(5);
	m_spinAtomSZ->setValue(6);
	m_spinAtomSMag->setValue(7);

	for(QSpinBox* spin : {m_spinAtomName, m_spinAtomX, m_spinAtomY, m_spinAtomZ,
		m_spinAtomSX, m_spinAtomSY, m_spinAtomSZ, m_spinAtomSMag})
	{
		// -1 means not used
		spin->setMinimum(-1);
	}

	QLabel *labelAtoms = new QLabel("Atom Positions Table:", this);
	m_editAtoms = new QTextEdit(this);
	m_editAtoms->setLineWrapMode(QTextEdit::NoWrap);

	QFrame *sep1 = new QFrame(this);
	sep1->setFrameStyle(QFrame::HLine);
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	QLabel *labelCouplingIdx = new QLabel("Column Indices in Magnetic Couplings Table:", this);
	m_spinCouplingName = new QSpinBox(this);
	m_spinCouplingAtom1 = new QSpinBox(this);
	m_spinCouplingAtom2 = new QSpinBox(this);
	m_checkIndices1Based = new QCheckBox(this);
	m_spinCouplingDX = new QSpinBox(this);
	m_spinCouplingDY = new QSpinBox(this);
	m_spinCouplingDZ = new QSpinBox(this);
	m_spinCouplingJ = new QSpinBox(this);
	m_spinCouplingDMIX = new QSpinBox(this);
	m_spinCouplingDMIY = new QSpinBox(this);
	m_spinCouplingDMIZ = new QSpinBox(this);

	m_spinCouplingName->setPrefix("name = ");
	m_spinCouplingAtom1->setPrefix("atom1 = ");
	m_spinCouplingAtom2->setPrefix("atom2 = ");
	m_spinCouplingDX->setPrefix("Δx = ");
	m_spinCouplingDY->setPrefix("Δy = ");
	m_spinCouplingDZ->setPrefix("Δz = ");
	m_spinCouplingJ->setPrefix("J = ");
	m_spinCouplingDMIX->setPrefix("DMIx = ");
	m_spinCouplingDMIY->setPrefix("DMIy = ");
	m_spinCouplingDMIZ->setPrefix("DMIz = ");
	m_checkIndices1Based->setText("1-Based");

	m_spinCouplingName->setValue(0);
	m_spinCouplingAtom1->setValue(1);
	m_spinCouplingAtom2->setValue(2);
	m_spinCouplingDX->setValue(3);
	m_spinCouplingDY->setValue(4);
	m_spinCouplingDZ->setValue(5);
	m_spinCouplingJ->setValue(6);
	m_spinCouplingDMIX->setValue(7);
	m_spinCouplingDMIY->setValue(8);
	m_spinCouplingDMIZ->setValue(9);
	m_checkIndices1Based->setChecked(false);

	for(QSpinBox* spin : {m_spinCouplingName, m_spinCouplingAtom1, m_spinCouplingAtom2,
		m_spinCouplingDX, m_spinCouplingDY, m_spinCouplingDZ,
		m_spinCouplingJ, m_spinCouplingDMIX, m_spinCouplingDMIY, m_spinCouplingDMIZ})
	{
		// -1 means not used
		spin->setMinimum(-1);
	}

	QLabel *labelCouplings = new QLabel("Magnetic Couplings Table:", this);
	m_editCouplings = new QTextEdit(this);
	m_editCouplings->setLineWrapMode(QTextEdit::NoWrap);
	// --------------------------------------------------------------------------------

	QFrame *sep2 = new QFrame(this);
	sep2->setFrameStyle(QFrame::HLine);

	QPushButton *btnImportAtoms = new QPushButton("Import Atoms", this);
	QPushButton *btnImportCouplings = new QPushButton("Import Couplings", this);
	QPushButton *btnOk = new QPushButton("Close", this);

	// grid
	QGridLayout* grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);
	int y = 0;
	grid->addWidget(labelAtomIdx, y++, 0, 1, 4);
	grid->addWidget(m_spinAtomName, y, 0, 1, 1);
	grid->addWidget(m_spinAtomX, y, 1, 1, 1);
	grid->addWidget(m_spinAtomY, y, 2, 1, 1);
	grid->addWidget(m_spinAtomZ, y++, 3, 1, 1);
	grid->addWidget(m_spinAtomSX, y, 0, 1, 1);
	grid->addWidget(m_spinAtomSY, y, 1, 1, 1);
	grid->addWidget(m_spinAtomSZ, y, 2, 1, 1);
	grid->addWidget(m_spinAtomSMag, y++, 3, 1, 1);
	grid->addWidget(labelAtoms, y++, 0, 1, 4);
	grid->addWidget(m_editAtoms, y++, 0, 1, 4);
	grid->addWidget(sep1, y++, 0, 1, 4);
	grid->addWidget(labelCouplingIdx, y++, 0, 1, 4);
	grid->addWidget(m_spinCouplingName, y, 0, 1, 1);
	grid->addWidget(m_spinCouplingAtom1, y, 1, 1, 1);
	grid->addWidget(m_spinCouplingAtom2, y, 2, 1, 1);
	grid->addWidget(m_checkIndices1Based, y++, 3, 1, 1);
	grid->addWidget(m_spinCouplingDX, y, 0, 1, 1);
	grid->addWidget(m_spinCouplingDY, y, 1, 1, 1);
	grid->addWidget(m_spinCouplingDZ, y++, 2, 1, 1);
	grid->addWidget(m_spinCouplingJ, y, 0, 1, 1);
	grid->addWidget(m_spinCouplingDMIX, y, 1, 1, 1);
	grid->addWidget(m_spinCouplingDMIY, y, 2, 1, 1);
	grid->addWidget(m_spinCouplingDMIZ, y++, 3, 1, 1);
	grid->addWidget(labelCouplings, y++, 0, 1, 4);
	grid->addWidget(m_editCouplings, y++, 0, 1, 4);
	grid->addWidget(sep2, y++, 0, 1, 4);
	grid->addWidget(btnImportAtoms, y, 0, 1, 1);
	grid->addWidget(btnImportCouplings, y, 1, 1, 1);
	grid->addWidget(btnOk, y++, 3, 1, 1);

	if(m_sett)
	{
		// restore window size and position
		if(m_sett->contains("tableimport/geo"))
			restoreGeometry(m_sett->value("tableimport/geo").toByteArray());
		else
			resize(500, 500);

		if(m_sett->contains("tableimport/idx_atom_name"))
			m_spinAtomName->setValue(m_sett->value("tableimport/idx_atom_name").toInt());
		if(m_sett->contains("tableimport/idx_atom_x"))
			m_spinAtomX->setValue(m_sett->value("tableimport/idx_atom_x").toInt());
		if(m_sett->contains("tableimport/idx_atom_y"))
			m_spinAtomY->setValue(m_sett->value("tableimport/idx_atom_y").toInt());
		if(m_sett->contains("tableimport/idx_atom_z"))
			m_spinAtomZ->setValue(m_sett->value("tableimport/idx_atom_z").toInt());
		if(m_sett->contains("tableimport/idx_atom_Sx"))
			m_spinAtomSX->setValue(m_sett->value("tableimport/idx_atom_Sx").toInt());
		if(m_sett->contains("tableimport/idx_atom_Sy"))
			m_spinAtomSY->setValue(m_sett->value("tableimport/idx_atom_Sy").toInt());
		if(m_sett->contains("tableimport/idx_atom_Sz"))
			m_spinAtomSZ->setValue(m_sett->value("tableimport/idx_atom_Sz").toInt());
		if(m_sett->contains("tableimport/idx_atom_Smag"))
			m_spinAtomSMag->setValue(m_sett->value("tableimport/idx_atom_Smag").toInt());

		if(m_sett->contains("tableimport/idx_coupling_name"))
			m_spinCouplingName->setValue(m_sett->value("tableimport/idx_coupling_name").toInt());
		if(m_sett->contains("tableimport/idx_coupling_atomidx_1"))
			m_spinCouplingAtom1->setValue(m_sett->value("tableimport/idx_coupling_atomidx_1").toInt());
		if(m_sett->contains("tableimport/idx_coupling_atomidx_2"))
			m_spinCouplingAtom2->setValue(m_sett->value("tableimport/idx_coupling_atomidx_2").toInt());
		if(m_sett->contains("tableimport/idx_coupling_Dx"))
			m_spinCouplingDX->setValue(m_sett->value("tableimport/idx_coupling_Dx").toInt());
		if(m_sett->contains("tableimport/idx_coupling_Dy"))
			m_spinCouplingDY->setValue(m_sett->value("tableimport/idx_coupling_Dy").toInt());
		if(m_sett->contains("tableimport/idx_coupling_Dz"))
			m_spinCouplingDZ->setValue(m_sett->value("tableimport/idx_coupling_Dz").toInt());
		if(m_sett->contains("tableimport/idx_coupling_J"))
			m_spinCouplingJ->setValue(m_sett->value("tableimport/idx_coupling_J").toInt());
		if(m_sett->contains("tableimport/idx_coupling_DMIx"))
			m_spinCouplingDMIX->setValue(m_sett->value("tableimport/idx_coupling_DMIx").toInt());
		if(m_sett->contains("tableimport/idx_coupling_DMIy"))
			m_spinCouplingDMIY->setValue(m_sett->value("tableimport/idx_coupling_DMIy").toInt());
		if(m_sett->contains("tableimport/idx_coupling_DMIz"))
			m_spinCouplingDMIZ->setValue(m_sett->value("tableimport/idx_coupling_DMIz").toInt());
		if(m_sett->contains("tableimport/idx_coupling_indices_1based"))
			m_checkIndices1Based->setChecked(m_sett->value("tableimport/idx_coupling_indices_1based").toBool());
	}

	// connections
	connect(btnImportAtoms, &QAbstractButton::clicked, this, &TableImportDlg::ImportAtoms);
	connect(btnImportCouplings, &QAbstractButton::clicked, this, &TableImportDlg::ImportCouplings);
	connect(btnOk, &QAbstractButton::clicked, this, &QDialog::close);
}



TableImportDlg::~TableImportDlg()
{
}



/**
 * read in the atoms from the table
 */
void TableImportDlg::ImportAtoms()
{
	std::string txt = m_editAtoms->toPlainText().toStdString();
	std::vector<std::string> lines;
	tl2::get_tokens<std::string, std::string>(txt, "\n", lines);

	const int idx_name = m_spinAtomName->value();
	const int idx_pos_x = m_spinAtomX->value();
	const int idx_pos_y = m_spinAtomY->value();
	const int idx_pos_z = m_spinAtomZ->value();
	const int idx_S_x = m_spinAtomSX->value();
	const int idx_S_y = m_spinAtomSY->value();
	const int idx_S_z = m_spinAtomSZ->value();
	const int idx_S_mag = m_spinAtomSMag->value();

	std::vector<TableImportAtom> atompos_vec;
	atompos_vec.reserve(lines.size());

	for(const std::string& line : lines)
	{
		std::vector<std::string> cols;
		tl2::get_tokens<std::string, std::string>(line, " \t", cols);

		TableImportAtom atompos;

		if(idx_name >= 0 && idx_name < int(cols.size()))
			atompos.name = cols[idx_name];

		if(idx_pos_x >= 0 && idx_pos_x < int(cols.size()))
			atompos.x = tl2::str_to_var<t_real>(cols[idx_pos_x]);
		if(idx_pos_y >= 0 && idx_pos_y < int(cols.size()))
			atompos.y = tl2::str_to_var<t_real>(cols[idx_pos_y]);
		if(idx_pos_z >= 0 && idx_pos_z < int(cols.size()))
			atompos.z = tl2::str_to_var<t_real>(cols[idx_pos_z]);

		if(idx_S_x >= 0 && idx_S_x < int(cols.size()))
			atompos.Sx = tl2::str_to_var<t_real>(cols[idx_S_x]);
		if(idx_S_y >= 0 && idx_S_y < int(cols.size()))
			atompos.Sy = tl2::str_to_var<t_real>(cols[idx_S_y]);
		if(idx_S_z >= 0 && idx_S_z < int(cols.size()))
			atompos.Sz = tl2::str_to_var<t_real>(cols[idx_S_z]);

		if(idx_S_mag >= 0 && idx_S_mag < int(cols.size()))
			atompos.Smag = tl2::str_to_var<t_real>(cols[idx_S_mag]);

		atompos_vec.emplace_back(std::move(atompos));
	}

	emit SetAtomsSignal(atompos_vec);
}



/**
 * read in the couplings from the table
 */
void TableImportDlg::ImportCouplings()
{
	std::string txt = m_editCouplings->toPlainText().toStdString();
	std::vector<std::string> lines;
	tl2::get_tokens<std::string, std::string>(txt, "\n", lines);

	const int idx_name = m_spinCouplingName->value();
	const int idx_atom1 = m_spinCouplingAtom1->value();
	const int idx_atom2 = m_spinCouplingAtom2->value();
	const int idx_dx = m_spinCouplingDX->value();
	const int idx_dy = m_spinCouplingDY->value();
	const int idx_dz = m_spinCouplingDZ->value();
	const int idx_J = m_spinCouplingJ->value();
	const int idx_dmix = m_spinCouplingDMIX->value();
	const int idx_dmiy = m_spinCouplingDMIY->value();
	const int idx_dmiz = m_spinCouplingDMIZ->value();
	const bool one_based = m_checkIndices1Based->isChecked();

	std::vector<TableImportCoupling> couplings;
	couplings.reserve(lines.size());

	for(const std::string& line : lines)
	{
		std::vector<std::string> cols;
		tl2::get_tokens<std::string, std::string>(line, " \t", cols);

		TableImportCoupling coupling;

		if(idx_name >= 0 && idx_name < int(cols.size()))
			coupling.name = cols[idx_name];
		if(idx_atom1 >= 0 && idx_atom1 < int(cols.size()))
		{
			coupling.atomidx1 = tl2::str_to_var<t_size>(cols[idx_atom1]);
			if(one_based)
				--*coupling.atomidx1;
		}
		if(idx_atom2 >= 0 && idx_atom2 < int(cols.size()))
		{
			coupling.atomidx2 = tl2::str_to_var<t_size>(cols[idx_atom2]);
			if(one_based)
				--*coupling.atomidx2;
		}
		if(idx_dx >= 0 && idx_dx < int(cols.size()))
			coupling.dx = tl2::str_to_var<t_real>(cols[idx_dx]);
		if(idx_dy >= 0 && idx_dy < int(cols.size()))
			coupling.dy = tl2::str_to_var<t_real>(cols[idx_dy]);
		if(idx_dz >= 0 && idx_dz < int(cols.size()))
			coupling.dz = tl2::str_to_var<t_real>(cols[idx_dz]);
		if(idx_J >= 0 && idx_J < int(cols.size()))
			coupling.J = tl2::str_to_var<t_real>(cols[idx_J]);
		if(idx_dmix >= 0 && idx_dmix < int(cols.size()))
			coupling.dmix = tl2::str_to_var<t_real>(cols[idx_dmix]);
		if(idx_dmiy >= 0 && idx_dmiy < int(cols.size()))
			coupling.dmiy = tl2::str_to_var<t_real>(cols[idx_dmiy]);
		if(idx_dmiz >= 0 && idx_dmiz < int(cols.size()))
			coupling.dmiz = tl2::str_to_var<t_real>(cols[idx_dmiz]);

		couplings.emplace_back(std::move(coupling));
	}

	emit SetCouplingsSignal(couplings);
}



/**
 * dialog is closing
 */
void TableImportDlg::closeEvent(QCloseEvent *)
{
	if(!m_sett)
		return;

	m_sett->setValue("tableimport/geo", saveGeometry());

	m_sett->setValue("tableimport/idx_atom_name", m_spinAtomName->value());
	m_sett->setValue("tableimport/idx_atom_x", m_spinAtomX->value());
	m_sett->setValue("tableimport/idx_atom_y", m_spinAtomY->value());
	m_sett->setValue("tableimport/idx_atom_z", m_spinAtomZ->value());
	m_sett->setValue("tableimport/idx_atom_Sx", m_spinAtomSX->value());
	m_sett->setValue("tableimport/idx_atom_Sy", m_spinAtomSY->value());
	m_sett->setValue("tableimport/idx_atom_Sz", m_spinAtomSZ->value());
	m_sett->setValue("tableimport/idx_atom_Smag", m_spinAtomSMag->value());

	m_sett->setValue("tableimport/idx_coupling_name", m_spinCouplingName->value());
	m_sett->setValue("tableimport/idx_coupling_atomidx_1", m_spinCouplingAtom1->value());
	m_sett->setValue("tableimport/idx_coupling_atomidx_2", m_spinCouplingAtom2->value());
	m_sett->setValue("tableimport/idx_coupling_Dx", m_spinCouplingDX->value());
	m_sett->setValue("tableimport/idx_coupling_Dy", m_spinCouplingDY->value());
	m_sett->setValue("tableimport/idx_coupling_Dz", m_spinCouplingDZ->value());
	m_sett->setValue("tableimport/idx_coupling_J", m_spinCouplingJ->value());
	m_sett->setValue("tableimport/idx_coupling_DMIx", m_spinCouplingDMIX->value());
	m_sett->setValue("tableimport/idx_coupling_DMIy", m_spinCouplingDMIY->value());
	m_sett->setValue("tableimport/idx_coupling_DMIz", m_spinCouplingDMIZ->value());
	m_sett->setValue("tableimport/idx_coupling_indices_1based", m_checkIndices1Based->isChecked());
}
