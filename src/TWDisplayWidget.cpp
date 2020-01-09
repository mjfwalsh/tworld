/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include "TWDisplayWidget.h"

#include <QPainter>

TWDisplayWidget::TWDisplayWidget(QWidget* pParent)
	:
	QWidget(pParent)
{
}


void TWDisplayWidget::setPixmap(const QPixmap& pixmap, double s)
{
	if(s > 0) scale = s;

	if(pixmap.width() < 1 || pixmap.height() < 1) return;

	QSize prevSize = m_pixmap.size();

	if(refSize.width() < 1 || refSize.height() < 1)
		refSize = pixmap.size();

	QSize newSize = refSize * scale;
	QSize oldSize = pixmap.size();
	if(newSize == oldSize) {
		m_pixmap = pixmap;
	} else {
		m_pixmap = pixmap.scaled(newSize, Qt::KeepAspectRatio);
	}

	newSize = m_pixmap.size();

	if(prevSize != newSize) updateGeometry();

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
