/**
 * numeric table widget item
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#ifndef __NUM_TABWIDGETITEM_H__
#define __NUM_TABWIDGETITEM_H__


#include <QtWidgets/QTableWidget>

#include <sstream>
#include <string>


template<class T = double>
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
	NumericTableWidgetItem(T&& val)
		: QTableWidgetItem(std::to_string(std::forward<T>(val)).c_str())
	{}
	NumericTableWidgetItem(const T& val)
		: QTableWidgetItem(std::to_string(val).c_str())
	{}

	NumericTableWidgetItem(const QString& val) : QTableWidgetItem(val)
	{}

	virtual bool operator<(const QTableWidgetItem& item) const override
	{
		T val1{}, val2{};
		std::istringstream{text().toStdString()} >> val1;
		std::istringstream{item.text().toStdString()} >> val2;

		return val1 < val2;
	}

	virtual QTableWidgetItem* clone() const override
	{
		auto item = new NumericTableWidgetItem<T>(this->text());
		item->setData(Qt::UserRole, this->data(Qt::UserRole));
		return item;
	};
};


#endif
