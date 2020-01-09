/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
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
	sText.replace("%b", QString::number(m_nPar));

	int diff = m_nValue - m_nPar;
	QString sign = diff < 0 ? "" : "+";
	sText.replace("%d", sign + QString::number(diff));
	return sText;
}

void TWProgressBar::paintBox(QPainter *p, int w, QColor bgcl, QColor fgcl, QString t)
{
	double fraction = (double)(w - minimum()) / (maximum() - minimum());
	int nRightLine = (int)(fraction * this->rect().width());

	if(m_nLeftLine == nRightLine) return;

	QRect box = this->rect();
	box.setLeft(m_nLeftLine);
	box.setRight(nRightLine);

	p->fillRect(box, bgcl);
	p->setClipRect(box);
	p->setPen(fgcl);
	p->drawText(this->rect(), Qt::AlignCenter, t);
	p->setClipping(false);

	m_nLeftLine = nRightLine;
}


void TWProgressBar::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::TextAntialiasing, false);

	QString t = text();
	m_nLeftLine = 0;
	
	if(isFullBar()) {
		paintBox(&painter, maximum(), QColor(0, 0, 0), QColor(255, 255, 255), t);
		return;
	}

	int nValue = value();
	int nPar = (par() > 0 && !m_bParBad) ? par() : 0;
	int nGreyArea = nValue < nPar ? nValue : nPar;

	paintBox(&painter, nGreyArea, QColor(70, 70, 70), QColor(255, 255, 255), t);
	paintBox(&painter, nValue, QColor(0, 0, 0), QColor(255, 255, 255), t);
	paintBox(&painter, maximum(), QColor(255, 255, 255), QColor(0, 0, 0), t);
}
