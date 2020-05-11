/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_TWTableSpec_h_
#define	HEADER_TWTableSpec_h_

#include <Qt>
#include <QAbstractTableModel>
#include <QString>
#include <QVector>

/* Qt align values.
 */
const int LeftAlign = (Qt::AlignLeft | Qt::AlignVCenter);
const int RightAlign = (Qt::AlignRight | Qt::AlignVCenter);
const int CenterAlign = (Qt::AlignHCenter | Qt::AlignVCenter);

class TWTableSpec : public QAbstractTableModel
{
public:
	explicit TWTableSpec();

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void addCell(QString text, int align = LeftAlign, int colspan = 1);

	void setCols(int c);
	void fixRows();

	void trimRows(int num);

	inline int cols() const
		{return m_nCols;}

protected:
	struct ItemInfo {
		int align;
		QString sText;
	};

	int m_nRows, m_nCols;
	QVector<ItemInfo> m_vecItems;

	QVariant GetData(int row, int col, int role) const;
};

#endif

