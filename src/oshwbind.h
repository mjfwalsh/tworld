/* oshwbind.h: Bridge to the Qt OS/hardware layer.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_qt_oshwbind_h_
#define	HEADER_qt_oshwbind_h_

#include <QPixmap>
#include <QImage>

struct gamestate;


/* Types
 */

typedef struct TW_Rect
{
	int x, y, w, h;

	TW_Rect() {}
	TW_Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
	explicit TW_Rect(const QRect& qr) : x(qr.x()), y(qr.y()), w(qr.width()), h(qr.height()) {}
	operator QRect() const {return QRect(x, y, w, h);}
} TW_Rect;


class Qt_Surface
{
public:
	Qt_Surface();
	Qt_Surface(int w, int h, bool bTransparent);
	explicit Qt_Surface(const char* szFilename);

	int w = 0;
	int h = 0;
	int pitch = 0;
	void* pixels;

	void SetPixmap(const QPixmap& pixmap);
	void SetImage(const QImage& image);

	const QPixmap& GetPixmap();

	void SwitchToPixmap();
	void SwitchToImage();

	void FillRect(const TW_Rect* pDstRect, uint32_t nColor);

	static void BlitSurface(Qt_Surface* pSrc, const TW_Rect* pSrcRect,
							Qt_Surface* pDst, const TW_Rect* pDstRect);

	void SetColorKey(uint32_t nColorKey);
	void ResetColorKey();

	inline bool IsColorKeySet() const
		{return m_bColorKeySet;}
	inline uint32_t GetColorKey() const
		{return m_nColorKey;}

	Qt_Surface* DisplayFormat();

	inline uint32_t PixelAt(int x, int y) const
	{
		return m_image.pixel(x, y);
		// TODO?: pixelIndex for 8-bit?
	}

private:
	QPixmap m_pixmap;
	QImage m_image;

	int bytesPerPixel = 0;
	int hasAlphaChannel = -1;
	bool m_bColorKeySet = false;
	uint32_t m_nColorKey = 0;

	void Init(const QPaintDevice& dev);
	void InitImage();
};


/*
 * Values global to this module. All the globals are placed in here,
 * in order to minimize pollution of the main module's namespace.
 */
typedef	struct genericglobals
{
	/*
	 * Shared variables.
	 */

	short		wtile;		/* width of one tile in pixels */
	short		htile;		/* height of one tile in pixels */
	Qt_Surface	       *screen;		/* the display */

	/* Coordinates of the NW corner of the visible part of the map
	 * (measured in quarter-tiles), or -1 if no map is currently visible.
	 */
	int			mapvieworigin;

} genericglobals;

/* generic module's structure of globals.
 */
extern genericglobals geng;

/* Functions
 */

extern uint32_t TW_MapRGB(uint8_t r, uint8_t g, uint8_t b);
extern uint32_t TW_MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Initialisation function
 */
extern void tileinitialize();

#endif
