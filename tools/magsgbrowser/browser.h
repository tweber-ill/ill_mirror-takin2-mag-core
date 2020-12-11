/**
 * space group browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date Apr-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#ifndef __SGBROWSE_H__
#define __SGBROWSE_H__


#include <QtCore/QSettings>
#include <QtGui/QGenericMatrix>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QListWidgetItem>

#include "tlibs2/libs/magsg.h"
#include "tlibs2/libs/math20.h"

#include "ui_browser.h"


using t_real_sg = double;
using t_vec_sg = tl2::qvec_adapter<int, 3, t_real_sg, QGenericMatrix>;
using t_mat_sg = tl2::qmat_adapter<int, 3, 3, t_real_sg, QGenericMatrix>;
using t_mat44_sg = tl2::qmat_adapter<int, 4, 4, t_real_sg, QGenericMatrix>;


class SgBrowserDlg : public QDialog, Ui::SgBrowserDlg
{
private:
	QSettings *m_pSettings = nullptr;

	Spacegroups<t_mat_sg, t_vec_sg> m_magsgs;
	std::vector<std::tuple<int, std::string, std::vector<t_mat44_sg>>> m_nuclsgs;

	bool m_showBNS = true;

private:
	void SetupMagSpaceGroups();
	void SetupNuclSpaceGroups();

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void hideEvent(QHideEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

	// slots
	void MagSpaceGroupSelected(QTreeWidgetItem *pItem);
	void NuclSpaceGroupSelected(QTreeWidgetItem *pItem);

	void SwitchToBNS(bool bBNS);

public:
	using QDialog::QDialog;
	SgBrowserDlg(QWidget* pParent = nullptr, QSettings* pSett = nullptr);
	~SgBrowserDlg() = default;
};


#endif
