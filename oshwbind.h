/* oshwbind.h: Binds the generic module to the Qt OS/hardware layer.
 *
 * Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_qt_oshwbind_h_
#define	HEADER_qt_oshwbind_h_


#include <stdint.h>


#ifdef __cplusplus
	#include <Qt>
	#include <QPixmap>
	#include <QImage>

	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif


/* Constants
 */

enum
{
	TW_ALPHA_TRANSPARENT = 0,
	TW_ALPHA_OPAQUE      = 255
};


#ifdef __cplusplus

enum
{
	TWK_LEFT = 1,
	TWK_UP,
	TWK_RIGHT,
	TWK_DOWN,
	TWK_RETURN,
	TWK_ESCAPE,
	TWK_HOME,
	TWK_END,
};

enum
{
	TWK_dummy = 10,

    TWC_SEESCORES,
    TWC_SEESOLUTIONFILES,
    TWC_TIMESCLIPBOARD,
    TWC_QUITLEVEL,
    TWC_QUIT,

    TWC_PROCEED,
    TWC_PAUSEGAME,
    TWC_SAMELEVEL,
    TWC_NEXTLEVEL,
    TWC_PREVLEVEL,
    TWC_GOTOLEVEL,

    TWC_PLAYBACK,
    TWC_CHECKSOLUTION,
    TWC_REPLSOLUTION,
    TWC_KILLSOLUTION,
    TWC_SEEK,

	TWK_LAST
};


enum
{
	TW_BUTTON_LEFT		= Qt::LeftButton,
	TW_BUTTON_RIGHT		= Qt::RightButton,
	TW_BUTTON_MIDDLE	= Qt::MidButton
};

#endif


/* Types
 */

typedef struct TW_Rect
{
	int x, y;
	int w, h;

#ifdef __cplusplus
	TW_Rect() {}
	TW_Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
	TW_Rect(const QRect& qr) : x(qr.x()), y(qr.y()), w(qr.width()), h(qr.height()) {}
	operator QRect() const {return QRect(x, y, w, h);}
#endif
} TW_Rect;


typedef struct TW_Surface
{
	int w, h;
	int bytesPerPixel;
	int pitch;
	void* pixels;
	int hasAlphaChannel;
} TW_Surface;


#ifdef __cplusplus

class Qt_Surface : public TW_Surface
{
public:
	Qt_Surface();

	void SetPixmap(const QPixmap& pixmap);
	void SetImage(const QImage& image);

	const QPixmap& GetPixmap();
	const QImage& GetImage();

	void Lock();
	void Unlock();

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

	bool m_bColorKeySet;
	uint32_t m_nColorKey;

	void Init(const QPaintDevice& dev);
	void InitImage();
};

#endif


/* Functions
 */

OSHW_EXTERN TW_Surface* TW_NewSurface(int w, int h, int bTransparent);
OSHW_EXTERN void TW_FreeSurface(TW_Surface* pSurface);

#define  TW_MUSTLOCK(pSurface)  1
OSHW_EXTERN void TW_LockSurface(TW_Surface* pSurface);
OSHW_EXTERN void TW_UnlockSurface(TW_Surface* pSurface);

OSHW_EXTERN void TW_FillRect(TW_Surface* pDst, const TW_Rect* pDstRect, uint32_t nColor);

OSHW_EXTERN int TW_BlitSurface(TW_Surface* pSrc, const TW_Rect* pSrcRect,
						       TW_Surface* pDst, const TW_Rect* pDstRect);
OSHW_EXTERN void TW_SetColorKey(TW_Surface* pSurface, uint32_t nColorKey);
OSHW_EXTERN void TW_ResetColorKey(TW_Surface* pSurface);

#define  TW_EnableAlpha(s)  ;
OSHW_EXTERN TW_Surface* TW_DisplayFormat(TW_Surface* pSurface);
OSHW_EXTERN TW_Surface* TW_DisplayFormatAlpha(TW_Surface* pSurface);

#define  TW_BytesPerPixel(pSurface)  ((pSurface)->bytesPerPixel)
OSHW_EXTERN uint32_t TW_PixelAt(const TW_Surface* pSurface, int x, int y);

OSHW_EXTERN uint32_t TW_MapRGB(uint8_t r, uint8_t g, uint8_t b);
OSHW_EXTERN uint32_t TW_MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

OSHW_EXTERN TW_Surface* TW_LoadBMP(const char* szFilename);

OSHW_EXTERN void TW_DebugSurface(TW_Surface* pSurface, const char* szFilename);	// @#$

#ifdef __cplusplus
OSHW_EXTERN bool* TW_GetKeyState();
#endif

OSHW_EXTERN uint32_t TW_GetTicks(void);
OSHW_EXTERN void TW_Delay(uint32_t nMS);

#define  TW_GetError()  "unspecified error"


#undef OSHW_EXTERN


#endif
