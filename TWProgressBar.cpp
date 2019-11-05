/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWProgressBar.h"

#include <QPainter>
#include <QPalette>


TWProgressBar::TWProgressBar(QWidget* pParent)
	:
	QProgressBar(pParent),
	m_nValue(0),
	m_nPar(-1),
	m_bParBad(false),
	m_bFullBar(false)
{
}


void TWProgressBar::setValue(int nValue)
{
	if (m_nValue == nValue) return;
	m_nValue = nValue;
	update();
}

void TWProgressBar::setPar(int nPar)
{
	if (m_nPar == nPar) return;
	m_nPar = nPar;
	update();
}

void TWProgressBar::setParBad(bool bParBad)
{
	if (m_bParBad == bParBad) return;
	m_bParBad = bParBad;
	update();
}

void TWProgressBar::setFullBar(bool bFullBar)
{
	if (m_bFullBar == bFullBar) return;
	m_bFullBar = bFullBar;
	update();
}

QString TWProgressBar::text() const
{
	QString sText = format();
	sText.replace("%v", QString::number(m_nValue));
	return sText;
}

void TWProgressBar::paintBox(QPainter *p, QRect container, QRect box, QColor bgcl, QColor fgcl, QString t)
{
	p->fillRect(box, bgcl);
	p->setClipRect(box);
	p->setPen(fgcl);
	p->drawText(container, Qt::AlignCenter, t);
	p->setClipping(false);
}


void TWProgressBar::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::TextAntialiasing, false);

	//const QPalette& pal = this->palette();
	QRect rect = this->rect();
	QString t = text();

	//static const int M = 2;

	//qDrawShadePanel(&painter, rect, pal, true, M);
	//rect.adjust(+M, +M, -M, -M);
	paintBox(&painter, rect, rect, QColor(255, 255, 255), QColor(0, 0, 0), t);

	int d = maximum() - minimum();
	double v;
	if (d > 0)
	{
		bool bHasPar = (par() > minimum());
		QRect rect2 = rect;
		v = ( isFullBar() ? 1.0 : (double(value() - minimum()) / d) );
		rect2.setWidth(int(v * rect.width()));

		QColor bc = (bHasPar && !m_bParBad) ? QColor(70, 70, 70) : QColor(0, 0, 0);
		paintBox(&painter, rect, rect2, bc, QColor(255, 255, 255), t);

		// qDrawShadePanel(&painter, rect2, pal, false, 1);
		if (bHasPar  &&  par() < value())
		{
			double p = double(par() - minimum()) / d;
			rect2.setLeft(int(p * rect.width()));

			paintBox(&painter, rect, rect2, QColor(0, 0, 0), QColor(255, 255, 255), t);
		}
	}
}
