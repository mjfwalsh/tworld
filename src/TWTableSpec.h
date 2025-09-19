/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef HEADER_TWTableSpec_h_
#define HEADER_TWTableSpec_h_

#include <QtCore/Qt>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QAction>
#include <QtWidgets/QMenu>

class TileWorldMainWnd;

/* Qt align values.
 */
const int LeftAlign = (Qt::AlignLeft | Qt::AlignVCenter);
const int RightAlign = (Qt::AlignRight | Qt::AlignVCenter);
const int CenterAlign = (Qt::AlignHCenter | Qt::AlignVCenter);

class TWTableSpec : public QAbstractTableModel
{
public:
    TWTableSpec(TileWorldMainWnd *parent);
    ~TWTableSpec();

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void addCell(QString text, int align = LeftAlign, int colspan = 1);

    void setCols(int c);
    void fixRows();

    void trimRows(int num);

    inline int cols() const
        {return m_cols;}

    void hideMenu(QMenu *menu);
    void hideAction(QAction *action);

protected:
    struct ItemInfo {
        int align;
        QString sText;
    };

    TileWorldMainWnd *m_parent;
    int m_rows, m_cols;
    QVector<ItemInfo> m_vecItems;

    QVariant GetData(int row, int col, int role) const;

    QVector<QAction*> m_hiddenactions;
    QVector<QMenu*> m_hiddenmenus;
};

#endif

