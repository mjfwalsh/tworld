/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QtCore/QString>

#include "TWTableSpec.h"
#include "TWMainWnd.h"

TWTableSpec::TWTableSpec(TileWorldMainWnd *parent)
    :
    m_parent(parent)
{
}

int TWTableSpec::rowCount(const QModelIndex& parent) const
{
    return m_rows-1;
}

int TWTableSpec::columnCount(const QModelIndex& parent) const
{
    return m_cols;
}

QVariant TWTableSpec::GetData(int row, int col, int role) const
{
    int i = row * m_cols + col;

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
    m_cols = c;
}

void TWTableSpec::fixRows()
{
    m_rows = m_vecItems.size() / m_cols;
}

void TWTableSpec::trimRows(int num)
{
    m_vecItems.erase(m_vecItems.end() - (num * m_cols), m_vecItems.end());
}


TWTableSpec::~TWTableSpec()
{
    m_parent->HideList();
}
