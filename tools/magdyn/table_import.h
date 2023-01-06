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

#ifndef __MAGDYN_TABLE_IMPORT_H__
#define __MAGDYN_TABLE_IMPORT_H__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSpinBox>

#include <optional>

#include "defs.h"



struct TableImportAtom
{
	std::optional<std::string> name{std::nullopt};
	std::optional<t_real> x{std::nullopt}, y{std::nullopt}, z{std::nullopt};
	std::optional<t_real> Sx{std::nullopt}, Sy{std::nullopt}, Sz{std::nullopt};
	std::optional<t_real> Smag{std::nullopt};
};



struct TableImportCoupling
{
	std::optional<std::string> name{std::nullopt};
	std::optional<t_size> atomidx1{std::nullopt}, atomidx2{std::nullopt};
	std::optional<t_real> dx{std::nullopt}, dy{std::nullopt}, dz{std::nullopt};
	std::optional<t_real> J{std::nullopt};
	std::optional<t_real> dmix{std::nullopt}, dmiy{std::nullopt}, dmiz{std::nullopt};
};



class TableImportDlg : public QDialog
{ Q_OBJECT
public:
	TableImportDlg(QWidget* parent = nullptr, QSettings* sett = nullptr);
	virtual ~TableImportDlg();

	TableImportDlg(const TableImportDlg&) = delete;
	const TableImportDlg& operator=(const TableImportDlg&) = delete;


protected:
	virtual void closeEvent(QCloseEvent *evt) override;

	void ImportAtoms();
	void ImportCouplings();


private:
	QSettings *m_sett{};

	// edit boxes
	QTextEdit *m_editAtoms{};
	QTextEdit *m_editCouplings{};

	// atom table column indices
	QSpinBox *m_spinAtomName{};
	QSpinBox *m_spinAtomX{}, *m_spinAtomY{}, *m_spinAtomZ{};
	QSpinBox *m_spinAtomSX{}, *m_spinAtomSY{}, *m_spinAtomSZ{};
	QSpinBox *m_spinAtomSMag{};

	// coupling table column
	QSpinBox *m_spinCouplingName{};
	QSpinBox *m_spinCouplingAtom1{}, *m_spinCouplingAtom2{};
	QCheckBox *m_checkIndices1Based{};
	QSpinBox *m_spinCouplingDX{}, *m_spinCouplingDY{}, *m_spinCouplingDZ{};
	QSpinBox *m_spinCouplingJ{};
	QSpinBox *m_spinCouplingDMIX{}, *m_spinCouplingDMIY{}, *m_spinCouplingDMIZ{};


signals:
	void SetAtomsSignal(const std::vector<TableImportAtom>&);
	void SetCouplingsSignal(const std::vector<TableImportCoupling>&);
};


#endif
