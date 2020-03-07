/* oshwbind.h: Bridge to the Qt OS/hardware layer.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag and Michael Walsh.
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

struct gamestate;

/* Constants
 */

enum
{
	TW_ALPHA_TRANSPARENT = 0,
	TW_ALPHA_OPAQUE      = 255
};

/* The dimensions of the visible area of the map (in tiles).
 */
#define	NXTILES		9
#define	NYTILES		9

/* The width/height of a tile in pixels at 100% zoom
 */
#define DEFAULTTILE		48

#ifdef __cplusplus

enum
{
	TWK_LEFT = 1,
	TWK_UP,
	TWK_RIGHT,
	TWK_DOWN,
#ifndef NDEBUG
	// NB: position important
	TWK_LEFT_CHEAT,
	TWK_UP_CHEAT,
	TWK_RIGHT_CHEAT,
	TWK_DOWN_CHEAT,
#endif
	TWK_RETURN,
	TWK_ESCAPE,

#ifndef NDEBUG
	TWK_DEBUG1,
	TWK_DEBUG2,
	TWK_CHIP,
	TWK_RED,
	TWK_BLUE,
	TWK_YELLOW,
	TWK_GREEN,
	TWK_ICE,
	TWK_SLIDE,
	TWK_FIRE,
	TWK_WATER,
#endif

	TWK_dummy,

	TWC_SEESCORES,
	TWC_SEESOLUTIONFILES,
	TWC_TIMESCLIPBOARD,
	TWC_QUITLEVEL,
	TWC_QUIT,

	TWC_PAUSEGAME,
	TWC_SAMELEVEL,
	TWC_NEXTLEVEL,
	TWC_PREVLEVEL,
	TWC_GOTOLEVEL,

	TWC_PLAYBACK,
	TWC_CHECKSOLUTION,
	TWC_DELSOLUTION,
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
	explicit TW_Rect(const QRect& qr) : x(qr.x()), y(qr.y()), w(qr.width()), h(qr.height()) {}
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
	TW_Surface	       *screen;		/* the display */

	/* Coordinates of the NW corner of the visible part of the map
	 * (measured in quarter-tiles), or -1 if no map is currently visible.
	 */
	int			mapvieworigin;

} genericglobals;

/* generic module's structure of globals.
 */
extern genericglobals geng;


#ifdef __cplusplus

class Qt_Surface : public TW_Surface
{
public:
	Qt_Surface();

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

OSHW_EXTERN void TW_SwitchSurfaceToImage(TW_Surface* pSurface);

OSHW_EXTERN void TW_FillRect(TW_Surface* pDst, const TW_Rect* pDstRect, uint32_t nColor);

OSHW_EXTERN int TW_BlitSurface(TW_Surface* pSrc, const TW_Rect* pSrcRect,
							   TW_Surface* pDst, const TW_Rect* pDstRect);
OSHW_EXTERN void TW_SetColorKey(TW_Surface* pSurface, uint32_t nColorKey);
OSHW_EXTERN void TW_ResetColorKey(TW_Surface* pSurface);

OSHW_EXTERN TW_Surface* TW_DisplayFormat(TW_Surface* pSurface);

OSHW_EXTERN uint32_t TW_PixelAt(const TW_Surface* pSurface, int x, int y);

OSHW_EXTERN uint32_t TW_MapRGB(uint8_t r, uint8_t g, uint8_t b);
OSHW_EXTERN uint32_t TW_MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

OSHW_EXTERN TW_Surface* TW_LoadBMP(const char* szFilename);


#define  TW_GetError()  "unspecified error"

/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
OSHW_EXTERN void eventupdate(int wait);

/* Render the view of the visible area of the map to the display, with
 * the view position centered on the display as much as possible. The
 * gamestate's map and the list of creatures are consulted to
 * determine what to render.
 */
OSHW_EXTERN void displaymapview(struct gamestate const *state, TW_Rect disploc);

/* Draw a tile of the given id at the position (xpos, ypos).
 */
OSHW_EXTERN void drawfulltileid(TW_Surface *dest, int xpos, int ypos, int id);

/* Initialisation function
 */
OSHW_EXTERN int tileinitialize();


#undef OSHW_EXTERN


#endif
