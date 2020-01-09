/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWPROGRESSBAR_H
#define TWPROGRESSBAR_H


#include <QProgressBar>
#include <qdrawutil.h>


// QProgressBar's setValue is slow enough to cause glitchy movement
//  during gameplay; so replace it with a simple implementation...

class TWProgressBar : public QProgressBar
{
public:
	TWProgressBar(QWidget* pParent = 0);

	// These aren't virtual, but we can still get by...
	void setValue(int nValue);
	int value() const
		{return m_nValue;}

	void setPar(int nPar);
	int par() const
		{return m_nPar;}

	void setParBad(bool bParBad);
	int isParBad() const
		{return m_bParBad;}

	void setFullBar(bool bFullBar);
	int isFullBar() const
		{return m_bFullBar;}

	virtual QString text() const;

protected:
	void paintBox(QPainter *p, int width, QColor bgcl, QColor fgcl, QString t);
	virtual void paintEvent(QPaintEvent* pPaintEvent);

	int m_nValue, m_nPar;
	bool m_bParBad;
	bool m_bFullBar;
	int m_nLeftLine = 0;
};


#endif
