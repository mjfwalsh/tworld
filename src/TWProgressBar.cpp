/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QtGui/QPainter>

#include "TWProgressBar.h"

TWProgressBar::TWProgressBar(QWidget* pParent)
    :
    QProgressBar(pParent)
{
}


void TWProgressBar::setValue(int nValue)
{
    if (m_value == nValue) return;
    m_value = nValue;
    update();
}

void TWProgressBar::setPar(int nPar)
{
    if (m_par == nPar) return;
    m_par = nPar;
    update();
}

void TWProgressBar::setParBad(bool bParBad)
{
    if (m_parBad == bParBad) return;
    m_parBad = bParBad;
    update();
}

void TWProgressBar::setFullBar(bool bFullBar)
{
    if (m_fullBar == bFullBar) return;
    m_fullBar = bFullBar;
    update();
}

QString TWProgressBar::text() const
{
    QString sText = format();
    sText.replace("%v", QString::number(m_value));
    sText.replace("%b", QString::number(m_par));

    int diff = m_value - m_par;
    QString sign = diff < 0 ? "" : "+";
    sText.replace("%d", sign + QString::number(diff));
    return sText;
}

void TWProgressBar::paintBox(QPainter *p, int w, QColor bgcl, QColor fgcl, QString t)
{
    double fraction = (double)(w - minimum()) / (maximum() - minimum());
    int nRightLine = (int)(fraction * this->rect().width());

    if(m_leftLine == nRightLine) return;

    QRect box = this->rect();
    box.setLeft(m_leftLine);
    box.setRight(nRightLine);

    p->fillRect(box, bgcl);
    p->setClipRect(box);
    p->setPen(fgcl);
    p->drawText(this->rect(), Qt::AlignCenter, t);
    p->setClipping(false);

    m_leftLine = nRightLine;
}


void TWProgressBar::paintEvent(QPaintEvent* pPaintEvent)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, false);

    QString t = text();
    m_leftLine = 0;

    if(isFullBar()) {
        paintBox(&painter, maximum(), QColor(0, 0, 0), QColor(255, 255, 255), t);
        return;
    }

    int nValue = value();
    int nPar = (par() > 0 && !m_parBad) ? par() : 0;
    int nGreyArea = nValue < nPar ? nValue : nPar;

    paintBox(&painter, nGreyArea, QColor(70, 70, 70), QColor(255, 255, 255), t);
    paintBox(&painter, nValue, QColor(0, 0, 0), QColor(255, 255, 255), t);
    paintBox(&painter, maximum(), QColor(255, 255, 255), QColor(0, 0, 0), t);
}
