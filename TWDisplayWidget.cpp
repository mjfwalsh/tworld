/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWDisplayWidget.h"

#include <QPainter>

TWDisplayWidget::TWDisplayWidget(QWidget* pParent)
	:
	QWidget(pParent)
{
}


void TWDisplayWidget::setPixmap(const QPixmap& pixmap)
{
	if(pixmap.width() < 1 || pixmap.height() < 1) return;

	QSize prevSize = m_pixmap.size();

	if(refSize.width() < 1 || refSize.height() < 1)
		refSize = pixmap.size();

	QSize vector = refSize * scale;
	m_pixmap = pixmap.scaled(vector, Qt::KeepAspectRatio);

	QSize newSize = m_pixmap.size();

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

void TWDisplayWidget::setScale(double s)
{
	if(scale == s) return;

	scale = s;

	if(refSize.width() < 1 || refSize.height() < 1 || m_pixmap.width() < 1 || m_pixmap.height() < 1) return;

	QSize vector = refSize * scale;
	m_pixmap = m_pixmap.scaled(vector, Qt::KeepAspectRatio);
	updateGeometry();
	repaint();
}