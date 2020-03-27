/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QVector>
#include <QString>

#include "TWTableSpec.h"

TWTableSpec::TWTableSpec(int c)
	:
	QAbstractTableModel(0),
	m_nRows(0), m_nCols(c)
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


void TWTableSpec::addCell(const char *text)
{
	m_vecItems.push_back({LeftAlign, QString(text)});
}

void TWTableSpec::addCell(QString text)
{
	m_vecItems.push_back({LeftAlign, text});
}


void TWTableSpec::addCell(int align, const char *text)
{
	m_vecItems.push_back({align, QString(text)});
}

void TWTableSpec::addCell(int align, QString text)
{
	m_vecItems.push_back({align, text});
}

void TWTableSpec::addCell(int colspan, int align, const char *text)
{
	m_vecItems.push_back({align, QString(text)});

	if(colspan > 1) {
		m_vecItems.resize(m_vecItems.size() + colspan - 1);
	}
}

void TWTableSpec::addCell(int colspan, int align, QString text)
{
	m_vecItems.push_back({align, text});

	if(colspan > 1) {
		m_vecItems.resize(m_vecItems.size() + colspan - 1);
	}
}

void TWTableSpec::fixRows()
{
	m_nRows = m_vecItems.size() / m_nCols;
}

void TWTableSpec::trimCells(int num)
{
	m_vecItems.erase(m_vecItems.end() - num, m_vecItems.end());
}
