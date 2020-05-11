/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QVector>
#include <QString>

#include "TWTableSpec.h"

TWTableSpec::TWTableSpec()
	:
	QAbstractTableModel(0),
	m_nRows(0), m_nCols(0)
{
}

int TWTableSpec::rowCount(const QModelIndex& parent) const
{
	return m_nRows-1;
}

int TWTableSpec::columnCount(const QModelIndex& parent) const
{
	return m_nCols;
}

QVariant TWTableSpec::GetData(int row, int col, int role) const
{
	int i = row * m_nCols + col;

	switch (role) {
		case Qt::DisplayRole:
			return m_vecItems[i].sText;

		case Qt::TextAlignmentRole:
			return m_vecItems[i].align;

		default:
			return QVariant();
	}
}

QVariant TWTableSpec::data(const QModelIndex& index, int role) const
{
	return GetData(1+index.row(), index.column(), role);
}

QVariant TWTableSpec::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
		return GetData(0, section, role);
	else
		return QVariant();
}

void TWTableSpec::addCell(QString text, int align, int colspan)
{
	int p = m_vecItems.size();
	m_vecItems.resize(p + colspan);

	if(align == RightAlign) {
		m_vecItems[p + colspan - 1] = {align, text};
	} else {
		m_vecItems[p] = {align, text};
	}
}

void TWTableSpec::setCols(int c)
{
	m_nCols = c;
}

void TWTableSpec::fixRows()
{
	m_nRows = m_vecItems.size() / m_nCols;
}

void TWTableSpec::trimRows(int num)
{
	m_vecItems.erase(m_vecItems.end() - (num * m_nCols), m_vecItems.end());
}
