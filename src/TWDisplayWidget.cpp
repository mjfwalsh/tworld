/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QPainter>

#include "TWDisplayWidget.h"

TWDisplayWidget::TWDisplayWidget(QWidget* pParent)
	:
	QWidget(pParent)
{
}


void TWDisplayWidget::setPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap.scaled(size(), Qt::KeepAspectRatio);
	repaint();
}

QSize TWDisplayWidget::sizeHint() const
{
	return m_pixmap.size();
}


void TWDisplayWidget::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_pixmap);
}
