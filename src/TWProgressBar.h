/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWPROGRESSBAR_H
#define TWPROGRESSBAR_H


#include <QtWidgets/QProgressBar>
#include <QtWidgets/qdrawutil.h>


// QProgressBar's setValue is slow enough to cause glitchy movement
//  during gameplay; so replace it with a simple implementation...

class TWProgressBar : public QProgressBar
{
public:
    TWProgressBar(QWidget* pParent = 0);

    // These aren't virtual, but we can still get by...
    void setValue(int nValue);
    int value() const
        {return m_value;}

    void setPar(int nPar);
    int par() const
        {return m_par;}

    void setParBad(bool bParBad);
    int isParBad() const
        {return m_parBad;}

    void setFullBar(bool bFullBar);
    int isFullBar() const
        {return m_fullBar;}

    virtual QString text() const;

protected:
    void paintBox(QPainter *p, int width, QColor bgcl, QColor fgcl, QString t);
    virtual void paintEvent(QPaintEvent* pPaintEvent);

    int m_value = 0;
    int m_par = -1;
    bool m_parBad = false;
    bool m_fullBar = false;
    int m_leftLine = 0;
};


#endif
