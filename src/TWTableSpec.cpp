/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QtCore/QString>

#include "TWTableSpec.h"
#include "TWMainWnd.h"

TWTableSpec::TWTableSpec(TileWorldMainWnd *parent)
    :
    QAbstractTableModel(0),
    m_parent(parent),
    m_rows(0), m_cols(0)
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

void TWTableSpec::hideMenu(QMenu *menu)
{
    if (menu->isEnabled()) {
        m_hiddenmenus.push_back(menu);
        menu->setEnabled(false);
    }
}

void TWTableSpec::hideAction(QAction *action)
{
    if (action->isEnabled()) {
        m_hiddenactions.push_back(action);
        action->setEnabled(false);
    }
}

TWTableSpec::~TWTableSpec()
{
    for (QAction *action : m_hiddenactions)
        action->setEnabled(true);

    for (QMenu *menu : m_hiddenmenus)
        menu->setEnabled(true);

    m_parent->HideList();
}