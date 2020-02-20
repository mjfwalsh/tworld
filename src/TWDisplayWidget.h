/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWDISPLAYWIDGET_H
#define TWDISPLAYWIDGET_H


#include <QWidget>
#include <QtGui/QPixmap>


// QLabel's setPixmap seems to trigger a re-layout of the parent
//  and hence a repaint which is particularly expensive with gradients
// Avoid doing that with this implementation...

class TWDisplayWidget : public QWidget
{
public:
	TWDisplayWidget(QWidget* pParent = 0);

	void setPixmap(const QPixmap& pixmap);
	const QPixmap* pixmap() const
		{return &m_pixmap;}

	virtual QSize sizeHint() const;

protected:
	virtual void paintEvent(QPaintEvent* pPaintEvent);

	QPixmap m_pixmap;
};


#endif
