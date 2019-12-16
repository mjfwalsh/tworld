/* Copyright (C) 2001-2019 by Madhav Shanbhag and Eric Schmidt.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QAbstractTableModel>

class TWStyledItemDelegate : public QStyledItemDelegate
{
public:
	TWStyledItemDelegate(QObject* pParent = 0)
		: QStyledItemDelegate(pParent) {}

	virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const;
};


void TWStyledItemDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& _option,
	const QModelIndex& index) const
{
	QStyleOptionViewItem option = _option;
	option.state &= ~QStyle::State_HasFocus;
	QStyledItemDelegate::paint(pPainter, option, index);
}

// ... All this just to remove a silly little dotted focus rectangle


class TWTableModel : public QAbstractTableModel
{
public:
	TWTableModel(QObject* pParent = 0);
	void SetTableSpec(const tablespec* pSpec);

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	struct ItemInfo
	{
		QString sText;
		Qt::Alignment align;
		ItemInfo() : align(Qt::AlignCenter) {}
	};

	int m_nRows, m_nCols;
	std::vector<ItemInfo> m_vecItems;

	QVariant GetData(int row, int col, int role) const;
};


TWTableModel::TWTableModel(QObject* pParent)
	:
	QAbstractTableModel(pParent),
	m_nRows(0), m_nCols(0)
{
}

void TWTableModel::SetTableSpec(const tablespec* pSpec)
{
	beginResetModel();

	m_nRows = pSpec->rows;
	m_nCols = pSpec->cols;
	int n = m_nRows * m_nCols;

	m_vecItems.clear();
	m_vecItems.reserve(n);

	ItemInfo dummyItemInfo;

	const char* const * pp = pSpec->items;
	for (int i = 0; i < n; ++pp)
	{
		const char* p = *pp;
		ItemInfo ii;

		ii.sText = p + 2;

		char c = p[1];
		Qt::Alignment ha = (c=='+' ? Qt::AlignRight : c=='.' ? Qt::AlignHCenter : Qt::AlignLeft);
		ii.align = (ha | Qt::AlignVCenter);

		m_vecItems.push_back(ii);

		int d = p[0] - '0';
		for (int j = 1; j < d; ++j)
		{
			m_vecItems.push_back(dummyItemInfo);
		}

		i += d;
	}

	//reset();
	endResetModel();
}

int TWTableModel::rowCount(const QModelIndex& parent) const
{
	return m_nRows-1;
}

int TWTableModel::columnCount(const QModelIndex& parent) const
{
	return m_nCols;
}

QVariant TWTableModel::GetData(int row, int col, int role) const
{
	int i = row*m_nCols + col;
	const ItemInfo& ii = m_vecItems[i];

	switch (role)
	{
		case Qt::DisplayRole:
			return ii.sText;

		case Qt::TextAlignmentRole:
			return int(ii.align);

		default:
			return QVariant();
	}
}

QVariant TWTableModel::data(const QModelIndex& index, int role) const
{
	return GetData(1+index.row(), index.column(), role);
}

QVariant TWTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
		return GetData(0, section, role);
	else
		return QVariant();
}
