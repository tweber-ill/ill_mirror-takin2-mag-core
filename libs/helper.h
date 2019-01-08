/**
 * tlibs2
 * helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TL2_HELPERS_H__
#define __TL2_HELPERS_H__

#include <QtCore/QLocale>
#include <locale>


namespace tl2 {


static inline void set_locales()
{
	std::ios_base::sync_with_stdio(false);

	::setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);
}


}
#endif
