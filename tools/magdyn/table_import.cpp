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

	// gui elements
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
		spin->setMinimum(-1);
	}

	QLabel *labelAtoms = new QLabel("Atom Positions Table:", this);
	m_editAtoms = new QTextEdit(this);

	QFrame *sep = new QFrame(this);
	sep->setFrameStyle(QFrame::HLine);

	QLabel *labelCouplings = new QLabel("Magnetic Couplings:", this);
	m_editCouplings = new QTextEdit(this);

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
	grid->addWidget(sep, y++, 0, 1, 3);
	grid->addWidget(labelCouplings, y++, 0, 1, 4);
	grid->addWidget(m_editCouplings, y++, 0, 1, 4);
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

	int idx_name = m_spinAtomName->value();
	int idx_pos_x = m_spinAtomX->value();
	int idx_pos_y = m_spinAtomY->value();
	int idx_pos_z = m_spinAtomZ->value();
	int idx_S_x = m_spinAtomSX->value();
	int idx_S_y = m_spinAtomSY->value();
	int idx_S_z = m_spinAtomSZ->value();
	int idx_S_mag = m_spinAtomSMag->value();

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
}
