/* oshwbind.cpp: Binds the generic module to the Qt OS/hardware layer.
 *
 * Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include <QBitmap>
#include <QPainter>

#include "oshwbind.h"

/* Values global to this module.
 */
genericglobals	geng;

Qt_Surface::Qt_Surface()
{
}

/* Create a fresh surface. If transparency is true, the surface is
 * created with 32-bit pixels, so as to ensure a complete alpha
 * channel. Otherwise, the surface is created with the same format as
 * the screen.
 */
Qt_Surface::Qt_Surface(int w, int h, bool bTransparent)
{
	if (bTransparent) {
		QImage image(w, h, QImage::Format_ARGB32);
		image.fill(0);
		this->SetImage(image);
	} else {
		QPixmap pixmap(w, h);
		pixmap.fill(Qt::black);
		this->SetPixmap(pixmap);
	}
}


/* Load the given bitmap file.
 */
Qt_Surface::Qt_Surface(const char* szFilename)
{
	QImage image(szFilename);
	if (image.isNull())
		return;

	image = image.convertToFormat(QImage::Format_ARGB32);
	// Doesn't seem to be necessary, but just in case...

	this->SetImage(image);

	// https://stackoverflow.com/questions/6157286/checking-if-a-qimage-has-an-alpha-channel
	this->hasAlphaChannel = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
	int bytes = image.sizeInBytes();
#else
	int bytes = image.byteCount();
#endif
	const void* image_pixels = image.bits();
	for (const QRgb* pixel = reinterpret_cast<const QRgb*>(image_pixels); bytes > 0; pixel++, bytes -= sizeof(QRgb)) {
		if (qAlpha(*pixel) != UCHAR_MAX) {
			this->hasAlphaChannel = 1;
			break;
		}
	}
}



void Qt_Surface::Init(const QPaintDevice& dev)
{
	w = dev.width();
	h = dev.height();
	bytesPerPixel = dev.depth() / 8;
	pitch = 0;
	pixels = 0;
}

void Qt_Surface::InitImage()
{
	bytesPerPixel = m_image.depth() / 8;
	pitch = m_image.bytesPerLine();
	pixels = m_image.bits();
}


void Qt_Surface::SetPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	m_image = QImage();
	Init(m_pixmap);
}

void Qt_Surface::SetImage(const QImage& image)
{
	m_image = image;
	m_pixmap = QPixmap();
	Init(m_image);
	InitImage();
}


const QPixmap& Qt_Surface::GetPixmap()
{
	SwitchToPixmap();
	return m_pixmap;
}

void Qt_Surface::SwitchToPixmap()
{
	if (m_pixmap.isNull()) {
		m_pixmap = QPixmap::fromImage(m_image);
		m_image = QImage();
	}
}

void Qt_Surface::SwitchToImage()
{
	if (m_image.isNull()) {
		m_image = m_pixmap.toImage();
		m_pixmap = QPixmap();
	}

	InitImage();
}

void Qt_Surface::FillRect(const TW_Rect* pDstRect, uint32_t nColor)
{
	SwitchToPixmap();
	// TODO?: don't force image -> pixmap?
	// TODO?: for 8-bit?
	if (!pDstRect) {
		m_pixmap.fill(nColor);
	} else {
		QPainter painter(&m_pixmap);
		painter.fillRect(*pDstRect, QColor(nColor));
	}

	pixels = 0;
}


void Qt_Surface::BlitSurface(Qt_Surface* pSrc, const TW_Rect* pSrcRect,
	Qt_Surface* pDst, const TW_Rect* pDstRect)
{
	TW_Rect srcRect = (pSrcRect ? *pSrcRect : TW_Rect(0,0, pSrc->w, pSrc->h));
	TW_Rect dstRect = (pDstRect ? *pDstRect : TW_Rect(0,0, pDst->w, pDst->h));
	if (srcRect.w == 0) srcRect.w = pSrc->w;
	if (srcRect.h == 0) srcRect.h = pSrc->h;
	if (dstRect.w == 0) dstRect.w = srcRect.w;
	if (dstRect.h == 0) dstRect.h = srcRect.h;
	if (!pDstRect) {
		dstRect.w = srcRect.w;
		dstRect.h = srcRect.h;
	} else if (!pSrcRect) {
		srcRect.w = dstRect.w;
		srcRect.h = dstRect.h;
	}

	// TODO?: don't force image -> pixmap?

	pDst->SwitchToPixmap();
	pDst->pixels = 0;

	QPixmap srcPix;
	if (pSrc->IsColorKeySet()) {
		srcPix = pSrc->GetPixmap().copy(srcRect);

		if(pSrc->hasAlphaChannel == 0) {
			QBitmap bmpMask = srcPix.createMaskFromColor(pSrc->GetColorKey());
			srcPix.setMask(bmpMask);
		}

		srcRect.x = srcRect.y = 0;

		pDst->m_pixmap = srcPix;	// @#$
		return;	// @#$
	} else {
		srcPix = pSrc->GetPixmap();
	}

	QPainter painter(&(pDst->m_pixmap));
	painter.drawPixmap(QRect(dstRect).topLeft(), srcPix, srcRect);
}


void Qt_Surface::SetColorKey(uint32_t nColorKey)
{
	m_nColorKey = nColorKey;
	m_bColorKeySet = true;
}

void Qt_Surface::ResetColorKey()
{
	m_bColorKeySet = false;
}


Qt_Surface* Qt_Surface::DisplayFormat()
{
	Qt_Surface* pNewSurface = new Qt_Surface(*this);
	if (!m_image.isNull())
		pNewSurface->pixels = pNewSurface->m_image.bits();
	pNewSurface->SwitchToPixmap();
	return pNewSurface;
}



uint32_t TW_MapRGB(uint8_t r, uint8_t g, uint8_t b)
{
	// TODO: for 8-bit
	return qRgb(r, g, b);
}


uint32_t TW_MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	// TODO: for 8-bit
	return qRgba(r, g, b, a);
}
