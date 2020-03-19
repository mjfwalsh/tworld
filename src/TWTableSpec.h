/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_TWTableSpec_h_
#define	HEADER_TWTableSpec_h_

#include <QAbstractTableModel>
#include <string>
#include <vector>

class TWTableSpec : public QAbstractTableModel
{
public:
	explicit TWTableSpec(int c);

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void addCell(std::string text);
	void addCell(const char *text);

	void addCell(int align, const char *text);
	void addCell(int align, std::string text);

	void addCell(int colspan, int align, const char *text);
	void addCell(int colspan, int align, std::string text);

	void fixRows();

	void trimCells(int num);

	inline int cols() const
		{return m_nCols;}

protected:
	struct ItemInfo {
		int align;
		QString sText;
	};

	int m_nRows, m_nCols;
	std::vector<ItemInfo> m_vecItems;

	QVariant GetData(int row, int col, int role) const;
};

#endif

