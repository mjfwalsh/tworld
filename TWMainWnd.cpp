/* Copyright (C) 2001-2017 by Madhav Shanbhag and Eric Schmidt
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWMainWnd.h"
#include "TWApp.h"

#include "generic.h"

#include "gen.h"
#include "defs.h"
#include "messages.h"
#include "settings.h"
#include "score.h"
#include "state.h"
#include "play.h"
#include "oshw.h"
#include "err.h"
#include "help.h"

extern int pedanticmode;

#include <QApplication>
#include <QClipboard>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QShortcut>

#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>

#include <QTextDocument>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include <QPainter>
#include <QPalette>
#include <QBrush>
#include <QLinearGradient>

#include <QDir>
#include <QFileInfo>

#include <QString>
#include <QTextStream>
#include <QTimer>

#include <vector>

#include <string.h>
#include <ctype.h>

using namespace std;

#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))

class TWStyledItemDelegate : public QStyledItemDelegate
{
public:
	TWStyledItemDelegate(QObject* pParent = 0)
		: QStyledItemDelegate(pParent) {}

	virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const;
};


void TWStyledItemDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& _option,
	const QModelIndex& index) const
{
	QStyleOptionViewItem option = _option;
	option.state &= ~QStyle::State_HasFocus;
	QStyledItemDelegate::paint(pPainter, option, index);
}

// ... All this just to remove a silly little dotted focus rectangle


class TWTableModel : public QAbstractTableModel
{
public:
	TWTableModel(QObject* pParent = 0);
	void SetTableSpec(const tablespec* pSpec);

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	struct ItemInfo
	{
		QString sText;
		Qt::Alignment align;
		ItemInfo() : align(Qt::AlignCenter) {}
	};

	int m_nRows, m_nCols;
	vector<ItemInfo> m_vecItems;

	QVariant GetData(int row, int col, int role) const;
};


TWTableModel::TWTableModel(QObject* pParent)
	:
	QAbstractTableModel(pParent),
	m_nRows(0), m_nCols(0)
{
}

void TWTableModel::SetTableSpec(const tablespec* pSpec)
{
	beginResetModel();

	m_nRows = pSpec->rows;
	m_nCols = pSpec->cols;
	int n = m_nRows * m_nCols;

	m_vecItems.clear();
	m_vecItems.reserve(n);

	ItemInfo dummyItemInfo;

	const char* const * pp = pSpec->items;
	for (int i = 0; i < n; ++pp)
	{
		const char* p = *pp;
		ItemInfo ii;

		ii.sText = p + 2;

		char c = p[1];
		Qt::Alignment ha = (c=='+' ? Qt::AlignRight : c=='.' ? Qt::AlignHCenter : Qt::AlignLeft);
		ii.align = (ha | Qt::AlignVCenter);

		m_vecItems.push_back(ii);

		int d = p[0] - '0';
		for (int j = 1; j < d; ++j)
		{
			m_vecItems.push_back(dummyItemInfo);
		}

		i += d;
	}

	//reset();
	endResetModel();
}

int TWTableModel::rowCount(const QModelIndex& parent) const
{
	return m_nRows-1;
}

int TWTableModel::columnCount(const QModelIndex& parent) const
{
	return m_nCols;
}

QVariant TWTableModel::GetData(int row, int col, int role) const
{
	int i = row*m_nCols + col;
	const ItemInfo& ii = m_vecItems[i];

	switch (role)
	{
		case Qt::DisplayRole:
			return ii.sText;

		case Qt::TextAlignmentRole:
			return int(ii.align);

		default:
			return QVariant();
	}
}

QVariant TWTableModel::data(const QModelIndex& index, int role) const
{
	return GetData(1+index.row(), index.column(), role);
}

QVariant TWTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
		return GetData(0, section, role);
	else
		return QVariant();
}


TileWorldMainWnd::TileWorldMainWnd(QWidget* pParent, Qt::WindowFlags flags)
	:
	QMainWindow(pParent, flags/*|Qt::FramelessWindowHint*/),
	m_bWindowClosed(false),
	m_pSurface(0),
	m_pInvSurface(0),
	m_bKbdRepeatEnabled(true),
	m_nRuleset(Ruleset_None),
	m_nLevelNum(0),
	m_nLevelName(""),
	m_bProblematic(false),
	m_bOFNT(false),
	m_nBestTime(TIME_NIL),
	m_hintMode(HINT_EMPTY),
	m_nTimeLeft(TIME_NIL),
	m_bTimedLevel(false),
	m_bReplay(false),
	m_pSortFilterProxyModel(0)
{
	memset(m_nKeyState, 0, TWK_LAST*sizeof(uint8_t));

	// load ui
	setupUi(this);

	// disable manual window resizing
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	// load style sheet
	QFile File("stylesheet.qss");
	File.open(QFile::ReadOnly);
	QString StyleSheet = QLatin1String(File.readAll());
	this->setStyleSheet(StyleSheet);

	m_pTblList->setItemDelegate( new TWStyledItemDelegate(m_pTblList) );

	//QStringList sPaths = {g_pApp->appDataDir, g_pApp->userDataDir};
	//m_pTextBrowser->setSearchPaths(sPaths);

	g_pApp->installEventFilter(this);

	connect( m_pTblList, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnListItemActivated(const QModelIndex&)) );
	connect( m_pRadioMs, SIGNAL(toggled(bool)), this, SLOT(OnRulesetSwitched(bool)) );
	connect( m_pTxtFind, SIGNAL(textChanged(const QString&)), this, SLOT(OnFindTextChanged(const QString&)) );
	connect( m_pTxtFind, SIGNAL(returnPressed()), this, SLOT(OnFindReturnPressed()) );
	connect( m_pBtnPlay, SIGNAL(clicked()), this, SLOT(OnPlayback()) );
	connect( m_pSldSpeed, SIGNAL(valueChanged(int)), this, SLOT(OnSpeedValueChanged(int)) );
	connect( m_pSldSpeed, SIGNAL(sliderReleased()), this, SLOT(OnSpeedSliderReleased()) );
	connect( m_pSldSeek, SIGNAL(valueChanged(int)), this, SLOT(OnSeekPosChanged(int)) );
	connect( m_pBtnTextNext, SIGNAL(clicked()), this, SLOT(OnTextNext()) );
	connect( m_pBtnTextPrev, SIGNAL(clicked()), this, SLOT(OnTextPrev()) );
	connect( m_pBtnTextReturn, SIGNAL(clicked()), this, SLOT(OnTextReturn()) );
	connect( m_pMenuBar, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionTriggered(QAction*)) );

	// change menu to reflect settings
	action_displayCCX->setChecked(getintsetting("displayccx"));
	action_forceShowTimer->setChecked(getintsetting("forceshowtimer") > 0);
	if (getintsetting("selectedruleset") == Ruleset_Lynx)
		m_pRadioLynx->setChecked(true);
	else
		m_pRadioMs->setChecked(true);

	// set a zoom menu item as checked
	int percentZoom = getintsetting("zoom");
	foreach (QAction *a, actiongroup_Zoom->actions()) {
		if(a->data() == percentZoom) {
			a->setChecked(true);
			break;
		}
	}
	if(actiongroup_Zoom->checkedAction() == nullptr) {
		action_Zoom100->setChecked(true);
	}

	int const tickMS = 1000 / TICKS_PER_SECOND;
	startTimer(tickMS / 2);

	// play pause icon for replay controls
	playIcon = QIcon("play.svg");
	pauseIcon = QIcon("pause.svg");

	// place window near top left corner
	move(30, 30);
	show();

	// timer for display of volume widegt
	volTimer = new QTimer(this);
}


TileWorldMainWnd::~TileWorldMainWnd()
{
	g_pApp->removeEventFilter(this);

	TW_FreeSurface(m_pInvSurface);
	TW_FreeSurface(m_pSurface);
}


void TileWorldMainWnd::closeEvent(QCloseEvent* pCloseEvent)
{
	QMainWindow::closeEvent(pCloseEvent);
	m_bWindowClosed = true;

	if (m_pMainWidget->currentIndex() == PAGE_GAME)
		g_pApp->ExitTWorld();
	else
		g_pApp->quit();
}

bool TileWorldMainWnd::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (HandleEvent(pObject, pEvent))
		return true;
	return QMainWindow::eventFilter(pObject, pEvent);
}

bool TileWorldMainWnd::HandleEvent(QObject* pObject, QEvent* pEvent)
{
	if (!isVisible()) return false;

	QEvent::Type eType = pEvent->type();
	// if (eType == QEvent::LayoutRequest) puts("QEvent::LayoutRequest");

	switch (eType)
	{
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
		{
			QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);

			int nQtKey = pKeyEvent->key();

			int nTWKey = -1;
			if (nQtKey >= 0x01000000  &&  nQtKey <= 0x01000060) {
				switch (nQtKey) {
					case Qt::Key_Return:
					case Qt::Key_Enter:  nTWKey = TWK_RETURN; break;
					case Qt::Key_Escape: nTWKey = TWK_ESCAPE; break;
					case Qt::Key_Up:     nTWKey = TWK_UP;     break;
					case Qt::Key_Left:   nTWKey = TWK_LEFT;   break;
					case Qt::Key_Down:   nTWKey = TWK_DOWN;   break;
					case Qt::Key_Right:  nTWKey = TWK_RIGHT;  break;
					case Qt::Key_Home:   nTWKey = TWK_HOME;   break;
					case Qt::Key_End:    nTWKey = TWK_END;    break;
					default: return false;
				}
			} else {
				return false;
			}

			// record key state
			bool bPress = (eType == QEvent::KeyPress);
			m_nKeyState[nTWKey] = bPress;

			bool bConsume = (m_pMainWidget->currentIndex() == PAGE_GAME) &&
							(QApplication::activeModalWidget() == 0);

			// List view
			QObjectList const & tableWidgets = m_pTablePage->children();
			if (bPress && tableWidgets.contains(pObject)) {
				int currentrow = m_pTblList->selectionModel()->currentIndex().row();
				if ((nTWKey == TWK_RETURN) && currentrow >= 0) {
					g_pApp->exit(CmdProceed);
					bConsume = true;
				} else if(nTWKey == TWK_ESCAPE) {
					g_pApp->exit(CmdQuitLevel);
					bConsume = true;
				} else {
					bConsume = false;
				}
			}

			// Text view
			if(m_pMainWidget->currentIndex() == PAGE_TEXT) {
				bConsume = true;
				if (nTWKey == TWK_RETURN) g_pApp->exit(+1);
				else if(nTWKey == TWK_ESCAPE) g_pApp->exit(CmdQuitLevel);
			}

			if (m_bKbdRepeatEnabled || !pKeyEvent->isAutoRepeat()) {
				keyeventcallback(nTWKey, bPress);
			}
			return bConsume;
		}
		break;

		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		{
			if (pObject != m_pGameWidget)
				return false;
			QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
			mouseeventcallback((int)pMouseEvent->x() / scale, (int)pMouseEvent->y() / scale, pMouseEvent->button(),
				(eType == QEvent::MouseButtonPress));
			return true;
		}
		break;

		default:
			break;
	}

	return false;
}


extern "C" uint8_t* TW_GetKeyState(int* pnNumKeys)
{
	return g_pMainWnd->GetKeyState(pnNumKeys);
}

uint8_t* TileWorldMainWnd::GetKeyState(int* pnNumKeys)
{
	if (pnNumKeys != 0)
		*pnNumKeys = TWK_LAST;
	return m_nKeyState;
}


void TileWorldMainWnd::PulseKey(int nTWKey)
{
	keyeventcallback(nTWKey, true);
	keyeventcallback(nTWKey, false);
}


void TileWorldMainWnd::OnPlayback()
{
	int nTWKey = m_bReplay ? TWC_PAUSEGAME : TWC_PLAYBACK;
	PulseKey(nTWKey);
}

/*
 * Keyboard input functions.
 */

/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
int setkeyboardrepeat(int enable)
{
	return g_pMainWnd->SetKeyboardRepeat(enable);
}

bool TileWorldMainWnd::SetKeyboardRepeat(bool bEnable)
{
	m_bKbdRepeatEnabled = bEnable;
	return true;
}


/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
int creategamedisplay(void)
{
	return g_pMainWnd->CreateGameDisplay();
}

bool TileWorldMainWnd::CreateGameDisplay()
{
	TW_FreeSurface(m_pSurface);
	TW_FreeSurface(m_pInvSurface);

	int w = NXTILES*geng.wtile, h = NYTILES*geng.htile;
	m_pSurface = static_cast<Qt_Surface*>(TW_NewSurface(w, h, false));
	m_pInvSurface = static_cast<Qt_Surface*>(TW_NewSurface(4*geng.wtile, 2*geng.htile, false));

	// set minimum sizes
	//m_pGameWidget->setMinimumSize(w, h);
	//m_pObjectsWidget->setMinimumSize(4*geng.wtile, 2*geng.htile);

	// get zoom
	int percentZoom = getintsetting("zoom");
	if(percentZoom == -1) percentZoom = 100;

	// this func also sets the pixmap
	this->SetScale(percentZoom, false);

	geng.screen = m_pSurface;
	m_disploc = TW_Rect(0, 0, w, h);
	geng.maploc = m_pGameWidget->geometry();

	SetCurrentPage(PAGE_GAME);

	m_pControlsFrame->setVisible(true);
	// this->adjustSize();
	// this->resize(minimumSizeHint());
	// TODO: not working!

	return true;
}


void TileWorldMainWnd::SetCurrentPage(Page ePage)
{
	m_pMainWidget->setCurrentIndex(ePage);
}


/* Fill the display with the background color.
 */
void cleardisplay(void)
{
	g_pMainWnd->ClearDisplay();
}

void TileWorldMainWnd::ClearDisplay()
{
	// TODO?
	geng.mapvieworigin = -1;
}


/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
int displaygame(gamestate const *state, int timeleft, int besttime)
{
	return g_pMainWnd->DisplayGame(state, timeleft, besttime);
}

bool TileWorldMainWnd::DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime)
{
	bool const bInit = (pState->currenttime == -1);
	bool const bTimedLevel = (pState->game->time > 0);

	m_nTimeLeft = nTimeLeft;
	m_bTimedLevel = bTimedLevel;

	bool const bForceShowTimer = action_forceShowTimer->isChecked();

	if (bInit)
	{
		m_nRuleset = pState->ruleset;
		m_nLevelNum = pState->game->number;
		m_bProblematic = false;
		m_nBestTime = nBestTime;
		m_bReplay = false;	// IMPORTANT for OnSpeedValueChanged
		SetSpeed(0);	// IMPORTANT

		m_pGameWidget->setCursor(m_nRuleset==Ruleset_MS ? Qt::CrossCursor : Qt::ArrowCursor);

		m_pLCDNumber->display(pState->game->number);

		m_nLevelName = pState->game->name;
		QString levelPackName(getstringsetting("selectedseries"));
		int p = levelPackName.indexOf(".");
		if(p > 0) levelPackName = levelPackName.left(p);
		m_pLblTitle->setText(levelPackName + " - " + m_nLevelName);

		Qt::AlignmentFlag halign = (m_pLblTitle->sizeHint().width() <= m_pLblTitle->width()) ? Qt::AlignHCenter : Qt::AlignLeft;
		m_pLblTitle->setAlignment(halign | Qt::AlignVCenter);

		m_pLblPassword->setText(pState->game->passwd);

		m_bOFNT = (m_nLevelName.toUpper() == "YOU CAN'T TEACH AN OLD FROG NEW TRICKS");

		m_pSldSeek->setValue(0);
		bool bHasSolution = hassolution(pState->game);
		m_pControlsFrame->setVisible(bHasSolution);

		menu_Game->setEnabled(true);
		menu_Solution->setEnabled(bHasSolution);
		menu_Help->setEnabled(true);
		action_GoTo->setEnabled(true);
                CCX::Level const & currLevel
		    (m_ccxLevelset.vecLevels[m_nLevelNum]);
		bool hasPrologue(!currLevel.txtPrologue.vecPages.empty());
		bool hasEpilogue(!currLevel.txtEpilogue.vecPages.empty());
		action_Prologue->setEnabled(hasPrologue);
		action_Epilogue->setEnabled(hasEpilogue && bHasSolution);

		m_pPrgTime->setPar(-1);

		bool bParBad = (pState->game->sgflags & SGF_REPLACEABLE) != 0;
		m_pPrgTime->setParBad(bParBad);
		const char* a = bParBad ? " *" : "";

		m_pPrgTime->setFullBar(!bTimedLevel);

		if (bTimedLevel)
		{
			if (nBestTime == TIME_NIL)
			{
				m_pPrgTime->setFormat("%v");
			}
			else
			{
				m_pPrgTime->setFormat(QString::number(nBestTime) + a + " / %v");
				m_pPrgTime->setPar(nBestTime);
				m_pSldSeek->setMaximum(nTimeLeft-nBestTime);
			}
			m_pPrgTime->setMaximum(pState->game->time);
			m_pPrgTime->setValue(nTimeLeft);
		}
		else
		{
			char const *noTime = (bForceShowTimer ? "[999]" : "---");
			if (nBestTime == TIME_NIL)
				m_pPrgTime->setFormat(noTime);
			else
			{
				m_pPrgTime->setFormat("[" + QString::number(nBestTime) + a + "] / " + noTime);
				m_pSldSeek->setMaximum(999-nBestTime);
			}
			m_pPrgTime->setMaximum(999);
			m_pPrgTime->setValue(999);
		}

		m_pLblHint->clear();
		m_pInfoPane->setCurrentIndex(0);
		SetHintMode(HINT_EMPTY);

		CheckForProblems(pState);

		Narrate(&CCX::Level::txtPrologue);
	}
	else
	{
		m_bReplay = (pState->replay >= 0);
		m_pControlsFrame->setVisible(m_bReplay);
		if (m_bProblematic)
		{
			m_pLblHint->clear();
			m_pInfoPane->setCurrentIndex(0);
			SetHintMode(HINT_EMPTY);
			m_bProblematic = false;
		}

		menu_Game->setEnabled(false);
		menu_Solution->setEnabled(false);
		menu_Help->setEnabled(false);
		action_GoTo->setEnabled(false);
		action_Prologue->setEnabled(false);
		action_Epilogue->setEnabled(false);
	}

	if (pState->statusflags & SF_SHUTTERED)
	{
		DisplayShutter();
	}
	else
	{
		DisplayMapView(pState);
	}

	for (int i = 0; i < 4; ++i)
	{
		drawfulltileid(m_pInvSurface, i*geng.wtile, 0,
			(pState->keys[i] ? Key_Red+i : Empty));
		drawfulltileid(m_pInvSurface, i*geng.wtile, geng.htile,
			(pState->boots[i] ? Boots_Ice+i : Empty));
	}
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	m_pLCDChipsLeft->display(pState->chipsneeded);

	m_pPrgTime->setValue(nTimeLeft);

	if (!bInit)
	{
		QString sFormat;
		std::string sFormatString;
		if (bTimedLevel)
			sFormatString = "%%v";
		else if (bForceShowTimer)
			sFormatString = "[%%v]";
		else
			sFormatString = "---";

		if ((bTimedLevel || bForceShowTimer) && nBestTime != TIME_NIL)
			sFormatString += " (%+d)";
		sFormat.sprintf(sFormatString.c_str(), nTimeLeft-nBestTime);
		m_pPrgTime->setFormat(sFormat);
	}

	if (m_bReplay && !m_pSldSeek->isSliderDown())
	{
		m_pSldSeek->blockSignals(true);
		m_pSldSeek->setValue(pState->currenttime / TICKS_PER_SECOND);
		m_pSldSeek->blockSignals(false);
	}

	if (!m_bProblematic)
	{
		// Call setText / clear only when really required
		// See comments about QLabel in TWDisplayWidget.h
		bool bShowHint = (pState->statusflags & SF_SHOWHINT) != 0;
		if (bShowHint)
		{
			if (SetHintMode(HINT_TEXT)) {
				m_pLblHint->setText(pState->hinttext);
				m_pInfoPane->setCurrentIndex(1);
			}
		}
		else if (SetHintMode(HINT_EMPTY)) {
			m_pLblHint->clear();
			m_pInfoPane->setCurrentIndex(0);
		}
	}

	return true;
}

void TileWorldMainWnd::CheckForProblems(const gamestate* pState)
{
	QString s;

	if (pState->statusflags & SF_INVALID)
	{
		s = "This level cannot be played.";
	}
	else if (pState->game->unsolvable)
	{
		s = "This level is reported to be unsolvable";
		if (*pState->game->unsolvable)
			s += ": " + QString(pState->game->unsolvable);
		s += ".";
	}
	else
	{
		CCX::RulesetCompatibility ruleCompat = m_ccxLevelset.vecLevels[m_nLevelNum].ruleCompat;
		CCX::Compatibility compat = CCX::COMPAT_UNKNOWN;
		if (m_nRuleset == Ruleset_Lynx)
		{
			if (pedanticmode)
				compat = ruleCompat.ePedantic;
			else
				compat = ruleCompat.eLynx;
		}
		else if (m_nRuleset == Ruleset_MS)
		{
			compat = ruleCompat.eMS;
		}
		if (compat == CCX::COMPAT_NO)
		{
			s = "This level is flagged as being incompatible with the current ruleset.";
		}
	}

	m_bProblematic = !s.isEmpty();
	if (m_bProblematic)
	{
		m_pLblHint->setText(s);
		m_pInfoPane->setCurrentIndex(1);
	}
}

void TileWorldMainWnd::DisplayMapView(const gamestate* pState)
{
	short xviewpos = pState->xviewpos;
	short yviewpos = pState->yviewpos;
	bool bFrogShow = (m_bOFNT  &&  m_bReplay  &&
	                  xviewpos/8 == 14  &&  yviewpos/8 == 9);
	if (bFrogShow)
	{
		int x = xviewpos, y = yviewpos;
		if (m_nRuleset == Ruleset_MS)
		{
			for (int pos = 0; pos < CXGRID*CYGRID; ++pos)
			{
				int id = pState->map[pos].top.id;
				if ( ! (id >= Teeth && id < Teeth+4) )
					continue;
				x = (pos % CXGRID) * 8;
				y = (pos / CXGRID) * 8;
				break;
			}
		}
		else
		{
			for (const creature* p = pState->creatures; p->id != 0; ++p)
			{
				if ( ! (p->id >= Teeth && p->id < Teeth+4) )
					continue;
				x = (p->pos % CXGRID) * 8;
				y = (p->pos / CXGRID) * 8;
				if (p->moving > 0)
				{
					switch (p->dir)
					{
					  case NORTH:	y += p->moving;	break;
					  case WEST:	x += p->moving;	break;
					  case SOUTH:	y -= p->moving;	break;
					  case EAST:	x -= p->moving;	break;
					}
				}
				break;
			}
		}
		const_cast<gamestate*>(pState)->xviewpos = x;
		const_cast<gamestate*>(pState)->yviewpos = y;
	}

	displaymapview(pState, m_disploc);
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());

	if (bFrogShow)
	{
		const_cast<gamestate*>(pState)->xviewpos = xviewpos;
		const_cast<gamestate*>(pState)->yviewpos = yviewpos;
	}
}

void TileWorldMainWnd::DisplayShutter()
{
	QPixmap pixmap(m_pGameWidget->size());
	pixmap.fill(Qt::black);

	QPainter painter(&pixmap);
	painter.setPen(Qt::red);
	QFont font;
	font.setPixelSize(geng.htile);
	painter.setFont(font);
	painter.drawText(pixmap.rect(), Qt::AlignCenter, "Paused");
	painter.end();

	m_pGameWidget->setPixmap(pixmap);
}


void TileWorldMainWnd::OnSpeedValueChanged(int nValue)
{
	// IMPORTANT!
	if (!m_bReplay) return;
	// Even though the replay controls are hidden when play begins,
	//  the slider could be manipulated before making the first move

	SetSpeed(nValue);
}

void TileWorldMainWnd::SetSpeed(int nValue)
{
	int nMS = (m_nRuleset == Ruleset_MS) ? 1100 : 1000;
	if (nValue >= 0)
		settimersecond(nMS >> nValue);
	else
		settimersecond(nMS << (-nValue/2));
}

void TileWorldMainWnd::OnSpeedSliderReleased()
{
	m_pSldSpeed->setValue(0);
}


/* Get number of seconds to skip at start of playback.
 */
int getreplaysecondstoskip(void)
{
	return g_pMainWnd->GetReplaySecondsToSkip();
}

int TileWorldMainWnd::GetReplaySecondsToSkip() const
{
	return m_pSldSeek->value();
}


void TileWorldMainWnd::OnSeekPosChanged(int nValue)
{
	PulseKey(TWC_SEEK);
}


/* Display a short message appropriate to the end of a level's game
 * play. If the level was completed successfully, completed is TRUE,
 * and the other three arguments define the base score and time bonus
 * for the level, and the user's total score for the series; these
 * scores will be displayed to the user.
 */
int displayendmessage(int basescore, int timescore, long totalscore,
			     int completed)
{
	return g_pMainWnd->DisplayEndMessage(basescore, timescore, totalscore, completed);
}

void TileWorldMainWnd::ReleaseAllKeys()
{
	// On X11, it seems that the last key-up event (for the arrow key that resulted in completion)
	//  is never sent (neither to the main widget, nor to the message box).
	// So pretend that all keys being held down were released.
	for (int k = 0; k < TWK_LAST; ++k)
	{
		if (m_nKeyState[k])
		{
			m_nKeyState[k] = false;
			keyeventcallback(k, false);
			// printf("*** RESET 0x%X\n", k);
		}
	}
}

int TileWorldMainWnd::DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted)
{
	if (nCompleted == 0)
		return CmdNone;

	if (nCompleted == -2)	// abandoned
		return CmdNone;

	QMessageBox msgBox(this);

	if (nCompleted > 0)	// Success
	{
		QString sAuthor = m_ccxLevelset.vecLevels[m_nLevelNum].sAuthor;
		const char* szMsg = 0;
		if (m_bReplay)
			szMsg = "Alright!";
		else
                {
			szMsg = getmessage(MessageWin);
                        if (!szMsg)
				szMsg = "You won!";
		}

		QString sText;
		QTextStream strm(&sText);
		strm.setLocale(m_locale);
		strm
			<< "<table>"
			// << "<tr><td><hr></td></tr>"
			<< "<tr><td><big><b>" << m_nLevelName << "</b></big></td></tr>"
			// << "<tr><td><hr></td></tr>"
			;
		if (!sAuthor.isEmpty())
			strm << "<tr><td>by " << sAuthor << "</td></tr>";
		strm
			<< "<tr><td><hr></td></tr>"
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><big><b>" << szMsg << "</b></big>" << "</td></tr>"
			;

		if (!m_bReplay)
		{
		  if (m_bTimedLevel  &&  m_nBestTime != TIME_NIL)
		  {
			strm << "<tr><td>";
			if (m_nTimeLeft > m_nBestTime)
			{
				strm << "You made it " << (m_nTimeLeft - m_nBestTime) << " second(s) faster this time!";
			}
			else if (m_nTimeLeft == m_nBestTime)
			{
				strm << "You scored " << m_nBestTime << " yet again.";
			}
			else
			{
				strm << "But not as quick as your previous score of " << m_nBestTime << "...";
			}
			strm << "</td></tr>";
		  }

		  strm
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><table>"
			  << "<tr><td>Time Bonus:</td><td align='right'>"  << nTimeScore << "</td></tr>"
			  << "<tr><td>Level Bonus:</td><td align='right'>" << nBaseScore << "</td></tr>"
			  << "<tr><td>Level Score:</td><td align='right'>" << (nTimeScore + nBaseScore) << "</td></tr>"
			  << "<tr><td colspan='2'><hr></td></tr>"
			  << "<tr><td>Total Score:</td><td align='right'>" << lTotalScore << "</td></tr>"
			<< "</table></td></tr>"
			// << "<tr><td><pre>                                </pre></td></tr>"	// spacer
			<< "</table>"
			;
		}

		msgBox.setTextFormat(Qt::RichText);
		msgBox.setText(sText);

		Qt_Surface* pSurface = static_cast<Qt_Surface*>(TW_NewSurface(geng.wtile, geng.htile, false));
		drawfulltileid(pSurface, 0, 0, Exited_Chip);
		msgBox.setIconPixmap(pSurface->GetPixmap());
		TW_FreeSurface(pSurface);

		msgBox.setWindowTitle(m_bReplay ? "Replay Completed" : "Level Completed");

		m_sTextToCopy = timestring
			(m_nLevelNum,
			 m_nLevelName.toLatin1().constData(),
			 m_nTimeLeft, m_bTimedLevel, false);

		msgBox.addButton("&Onward!", QMessageBox::AcceptRole);
		QPushButton* pBtnRestart = msgBox.addButton("&Restart", QMessageBox::AcceptRole);
		QPushButton* pBtnCopyScore = msgBox.addButton("&Copy Score", QMessageBox::ActionRole);
		connect( pBtnCopyScore, SIGNAL(clicked()), this, SLOT(OnCopyText()) );

		msgBox.exec();
		ReleaseAllKeys();
		if (msgBox.clickedButton() == pBtnRestart)
			return CmdSameLevel;

		// if (!m_bReplay)
			Narrate(&CCX::Level::txtEpilogue);
	}
	else	// Failure
	{
		bool bTimeout = (m_bTimedLevel  &&  m_nTimeLeft <= 0);
		if (m_bReplay)
		{
			QString sMsg = "Whoa!  Chip ";
			if (bTimeout)
				sMsg += "ran out of time";
			else
				sMsg += "ran into some trouble";
			// TODO: What about when Chip just doesn't reach the exit or reaches the exit too early?
			sMsg += " there.\nIt looks like the level has changed after that solution was recorded.";
			msgBox.setText(sMsg);
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle("Replay Failed");
		}
		else
		{
			const char* szMsg = 0;
			if (bTimeout)
			{
				szMsg = getmessage(MessageTime);
				if (!szMsg)
					szMsg = "You ran out of time.";
			}
			else
			{
				szMsg = getmessage(MessageDie);
				if (!szMsg)
					szMsg = "You died.";
			}

			msgBox.setTextFormat(Qt::PlainText);
			msgBox.setText(szMsg);
			// setIcon also causes the corresponding system sound to play
			// setIconPixmap does not
			QStyle* pStyle = QApplication::style();
			if (pStyle != 0)
			{
				QIcon icon = pStyle->standardIcon(QStyle::SP_MessageBoxWarning);
				msgBox.setIconPixmap(icon.pixmap(48));
			}
			msgBox.setWindowTitle("Oops.");
		}
		msgBox.exec();
		ReleaseAllKeys();
	}

	return CmdProceed;
}


/* Display a scrollable table. title provides a title to display. The
 * table's first row provides a set of column headers which will not
 * scroll. index points to the index of the item to be initially
 * selected; upon return, the value will hold the current selection.
 * Either listtype or inputcallback must be used to tailor the UI.
 * listtype specifies the type of list being displayed.
 * inputcallback points to a function that is called to retrieve
 * input. The function is passed a pointer to an integer. If the
 * callback returns TRUE, this integer should be set to either a new
 * index value or one of the following enum values. This value will
 * then cause the selection to be changed, whereupon the display will
 * be updated before the callback is called again. If the callback
 * returns FALSE, the table is removed from the display, and the value
 * stored in the integer will become displaylist()'s return value.
 */
int displaylist(tablespec const *table, int *index,
		       DisplayListType listtype)
{
	return g_pMainWnd->DisplayList(table, index, listtype);
}

int TileWorldMainWnd::DisplayList(const tablespec* pTableSpec, int* pnIndex,
		DisplayListType eListType)
{
  int nCmd = 0;

  // dummy scope to force model destructors before ExitTWorld
  {
	TWTableModel model;
	model.SetTableSpec(pTableSpec);
	QSortFilterProxyModel proxyModel;
	m_pSortFilterProxyModel = &proxyModel;
	proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	proxyModel.setFilterKeyColumn(-1);
	proxyModel.setSourceModel(&model);
	m_pTblList->setModel(&proxyModel);

	QModelIndex index = proxyModel.mapFromSource(model.index(*pnIndex, 0));
	m_pTblList->setCurrentIndex(index);
	m_pTblList->resizeColumnsToContents();
	m_pTblList->resizeRowsToContents();
	m_pTxtFind->clear();
	SetCurrentPage(PAGE_TABLE);
	m_pTblList->setFocus();

	bool const showRulesetOptions = (eListType == LIST_MAPFILES);
	m_pRadioMs->setVisible(showRulesetOptions);
	m_pRadioLynx->setVisible(showRulesetOptions);
	m_pLblSpace->setVisible(showRulesetOptions);

	nCmd = g_pApp->exec();

	*pnIndex = proxyModel.mapToSource(m_pTblList->currentIndex()).row();

	SetCurrentPage(PAGE_GAME);
	m_pTblList->setModel(0);
	m_pSortFilterProxyModel = 0;
  }

  if (m_bWindowClosed) g_pApp->ExitTWorld();

  return nCmd;
}

void TileWorldMainWnd::OnListItemActivated(const QModelIndex& index)
{
	g_pApp->exit(CmdProceed);
}

void TileWorldMainWnd::OnFindTextChanged(const QString& sText)
{
	if (!m_pSortFilterProxyModel) return;

	QString sWildcard;
	if (sText.isEmpty())
		sWildcard = "*";
	else
		sWildcard = '*' + sText + '*';
	m_pSortFilterProxyModel->setFilterWildcard(sWildcard);
}

void TileWorldMainWnd::OnFindReturnPressed()
{
	if (!m_pSortFilterProxyModel) return;

	int n = m_pSortFilterProxyModel->rowCount();
	if (n == 0)
	{
		ding();
		return;
	}

	m_pTblList->setFocus();

	if (!m_pTblList->currentIndex().isValid())
		m_pTblList->selectRow(0);

	if (n == 1)
		g_pApp->exit(CmdProceed);
}

void TileWorldMainWnd::OnRulesetSwitched(bool mschecked) {
	setintsetting("selectedruleset", mschecked ? Ruleset_MS : Ruleset_Lynx);
}

/* Display an input prompt to the user. prompt supplies the prompt to
 * display, and input points to a buffer to hold the user's input.
 * maxlen sets a maximum length to the input that will be accepted.
 * Either inputtype or inputcallback must be used to validate input.
 * inputtype indicates the type of input desired.
 * The supplied callback function is called repeatedly to obtain
 * input. If the callback function returns a printable ASCII
 * character, the function will automatically append it to the string
 * stored in input. If '\b' is returned, the function will erase the
 * last character in input, if any. If '\f' is returned the function
 * will set input to "". If '\n' is returned, the input prompt is
 * erased and displayinputprompt() returns TRUE. If a negative value
 * is returned, the input prompt is erased and displayinputprompt()
 * returns FALSE. All other return values from the callback are
 * ignored.
 */
int displayyesnoprompt(char const *prompt)
{
	return (int)g_pMainWnd->DisplayYesNoPrompt(prompt);
}

bool TileWorldMainWnd::DisplayYesNoPrompt(const char* prompt)
{
	QMessageBox::StandardButton eBtn = QMessageBox::question(
		this, TileWorldApp::applicationName(), prompt, QMessageBox::Yes|QMessageBox::No);
	return eBtn == QMessageBox::Yes;
}


const char *displaypasswordprompt()
{
	QString p = g_pMainWnd->DisplayPasswordPrompt();
	return p.toStdString().c_str();
}

QString TileWorldMainWnd::DisplayPasswordPrompt()
{
	QString password = QInputDialog::getText(this, TileWorldApp::applicationName(), "Enter Password");
	if (password.isEmpty()) return "";
	password.truncate(4);
	password = password.toUpper();
	return password;
}

/*
 * Miscellaneous functions.
 */

/* Ring the bell.
 */
void ding(void)
{
	QApplication::beep();
}


/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
void setsubtitle(char const *subtitle)
{
	g_pMainWnd->SetSubtitle(subtitle);
}

void TileWorldMainWnd::SetSubtitle(const char* szSubtitle)
{
	QString sTitle = TileWorldApp::applicationName();
	if (szSubtitle && *szSubtitle)
		sTitle += " - " + QString(szSubtitle);
	setWindowTitle(sTitle);
}


/* Display a message to the user. cfile and lineno can be NULL and 0
 * respectively; otherwise, they identify the source code location
 * where this function was called from. prefix is an optional string
 * that is displayed before and/or apart from the body of the message.
 * fmt and args define the formatted text of the message body. action
 * indicates how the message should be presented. NOTIFY_LOG causes
 * the message to be displayed in a way that does not interfere with
 * the program's other activities. NOTIFY_ERR presents the message as
 * an error condition. NOTIFY_DIE should indicate to the user that the
 * program is about to shut down.
 */
void usermessage(int action, char const *prefix,
                 char const *cfile, unsigned long lineno,
                 char const *fmt, va_list args)
{
	fprintf(stderr, "%s: ", action == NOTIFY_DIE ? "FATAL" :
	                        action == NOTIFY_ERR ? "error" : "warning");
	if (prefix)
		fprintf(stderr, "%s: ", prefix);
	if (fmt)
		vfprintf(stderr, fmt, args);
	if (cfile)
		fprintf(stderr, " [%s:%lu] ", cfile, lineno);
    fputc('\n', stderr);
    fflush(stderr);
}

int getselectedruleset()
{
	return g_pMainWnd->GetSelectedRuleset();
}

int TileWorldMainWnd::GetSelectedRuleset()
{
	return m_pRadioMs->isChecked() ? Ruleset_MS : Ruleset_Lynx;
}

/* Read any additional data for the series.
 */
void readextensions(gameseries *series)
{
	if (g_pMainWnd == 0) return;	// happens during batch verify, etc.
	g_pMainWnd->ReadExtensions(series);
}

void TileWorldMainWnd::ReadExtensions(gameseries* pSeries)
{
	QDir dataDir;
	QFile sFile;
	QString sFilePath;

	QString sSetName = QFileInfo(pSeries->mapfilename).completeBaseName();

	dataDir.setPath(g_pApp->appDataDir);
	sFilePath = dataDir.filePath(sSetName + ".ccx");
	sFile.setFileName(sFilePath);

	if(!sFile.exists()) {
		dataDir.setPath(g_pApp->userDataDir);
		sFilePath = dataDir.filePath(sSetName + ".ccx");
	}

	m_ccxLevelset.Clear();
	if (!m_ccxLevelset.ReadFile(sFilePath, pSeries->count))
		warn("%s: failed to read file", sFilePath.toLatin1().constData());

	for (int i = 1; i <= pSeries->count; ++i)
	{
		CCX::Level& rCCXLevel = m_ccxLevelset.vecLevels[i];
		rCCXLevel.txtPrologue.bSeen = false;	// @#$ (pSeries->games[i-1].sgflags & SGF_HASPASSWD) != 0;
		rCCXLevel.txtEpilogue.bSeen = false;
	}
}


void TileWorldMainWnd::Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce)
{
	CCX::Text& rText = m_ccxLevelset.vecLevels[m_nLevelNum].*pmTxt;
	if ((rText.bSeen || !action_displayCCX->isChecked()) && !bForce)
		return;
	rText.bSeen = true;

	if (rText.vecPages.empty())
		return;
	int n = rText.vecPages.size();

	QString sWindowTitle = this->windowTitle();
	SetSubtitle("");	// TODO: set name
	SetCurrentPage(PAGE_TEXT);
	m_pBtnTextNext->setFocus();

	int d = +1;
	for (int nPage = 0; nPage < n; nPage += d)
	{
		m_pBtnTextPrev->setVisible(nPage > 0);

		CCX::Page& rPage = rText.vecPages[nPage];

		QTextDocument* pDoc = m_pTextBrowser->document();
		if (pDoc != 0)
		{
			if (!m_ccxLevelset.sStyleSheet.isEmpty())
				pDoc->setDefaultStyleSheet(m_ccxLevelset.sStyleSheet);
			pDoc->setDocumentMargin(16);
		}

		QString sText = rPage.sText;
		if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN)
		{
			m_pTextBrowser->setPlainText(sText);
		}
		else
		{
			m_pTextBrowser->setHtml(sText);
		}

		d = g_pApp->exec();
		if (m_bWindowClosed) g_pApp->ExitTWorld();
		if (d == 0)	// Return
			break;
		if (nPage+d < 0)
			d = 0;
	}

	SetCurrentPage(PAGE_GAME);
	setWindowTitle(sWindowTitle);
}

void TileWorldMainWnd::ShowAbout()
{
	QString text;
	int const numlines = vourzhon->rows;
	for (int i = 0; i < numlines; ++i)
	{
		if (i > 0)
			text += "\n\n";
	    char const *item = vourzhon->items[i];
		text += (item + 2);  // skip over formatting chars
	}
	QMessageBox::about(this, "About", text);
}

void TileWorldMainWnd::OnTextNext()
{
	g_pApp->exit(+1);
}

void TileWorldMainWnd::OnTextPrev()
{
	g_pApp->exit(-1);
}

void TileWorldMainWnd::OnTextReturn()
{
	g_pApp->exit(0);
}


void TileWorldMainWnd::OnCopyText()
{
	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard == 0) return;
	pClipboard->setText(m_sTextToCopy);
}

void TileWorldMainWnd::OnMenuActionTriggered(QAction* pAction)
{
	if (pAction == action_Prologue)
	    { Narrate(&CCX::Level::txtPrologue, true); return; }
	if (pAction == action_Epilogue)
	    { Narrate(&CCX::Level::txtEpilogue, true); return; }

	if (pAction == action_displayCCX)
	{
	    setintsetting("displayccx", pAction->isChecked() ? 1 : 0);
	    return;
	}

	if (pAction == action_forceShowTimer)
	{
	    setintsetting("forceshowtimer", pAction->isChecked() ? 1 : 0);
		drawscreen(TRUE);
	    return;
	}

	if (pAction == action_About)
	{
		ShowAbout();
		return;
	}

	if (pAction->actionGroup()  == actiongroup_Zoom)
	{
		int s = pAction->data().toInt();
		setintsetting("zoom", s);
		this->SetScale(s);
		return;
	}

	if (pAction == action_VolumeUp)
	{
		this->ChangeVolume(+2);
		return;
	}

	if (pAction == action_VolumeDown)
	{
		this->ChangeVolume(-2);
		return;
	}
	
	if (pAction == action_Step)
	{
		// step dialog
		QInputDialog stepDialog(this);
		stepDialog.setWindowTitle("Step");
		stepDialog.setLabelText("Set level step value");

		if (getintsetting("selectedruleset") == Ruleset_Lynx) {
			stepDialog.setComboBoxItems(stepDialogOptions);
		} else {
			stepDialog.setComboBoxItems({stepDialogOptions[0], stepDialogOptions[4]});
		}

		int step = getstepping();
		stepDialog.setTextValue(stepDialogOptions[step]);

		stepDialog.exec();

		int stepIndex = stepDialogOptions.indexOf(stepDialog.textValue());
		setstepping(stepIndex);
		return;
	}

	int nTWKey = GetTWKeyForAction(pAction);
	if (nTWKey == TWK_dummy) return;
	PulseKey(nTWKey);
}

int TileWorldMainWnd::GetTWKeyForAction(QAction* pAction) const
{
    if (pAction == action_Scores) return TWC_SEESCORES;
    if (pAction == action_SolutionFiles) return TWC_SEESOLUTIONFILES;
    if (pAction == action_TimesClipboard) return TWC_TIMESCLIPBOARD;
    if (pAction == action_Levelsets) return TWC_QUITLEVEL;
    if (pAction == action_Exit) return TWC_QUIT;

    if (pAction == action_Begin) return TWC_PROCEED;
    if (pAction == action_Pause) return TWC_PAUSEGAME;
    if (pAction == action_Restart) return TWC_SAMELEVEL;
    if (pAction == action_Next) return TWC_NEXTLEVEL;
    if (pAction == action_Previous) return TWC_PREVLEVEL;
    if (pAction == action_GoTo) return TWC_GOTOLEVEL;

    if (pAction == action_Playback) return TWC_PLAYBACK;
    if (pAction == action_Verify) return TWC_CHECKSOLUTION;
    if (pAction == action_Replace) return TWC_REPLSOLUTION;
    if (pAction == action_Delete) return TWC_KILLSOLUTION;

    return TWK_dummy;
}

bool TileWorldMainWnd::SetHintMode(HintMode newmode)
{
	bool changed = (newmode != m_hintMode);
	m_hintMode = newmode;
	return changed;
}

void TileWorldMainWnd::SetScale(int s, bool checkPrevScale)
{
	double newScale = (double)s / 100;
	if(checkPrevScale && newScale == scale) return;

	scale = newScale;

	if(m_pSurface == 0 || m_pInvSurface == 0 || geng.wtile < 1) {
		warn("Attempt to set pixmap and scale without setting pixmap first");
		return;
	}

	// this sets the game and objects box
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap(), scale);
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap(), scale);

	// this aligns the width of the objects box with the other elements
	// in the right column
	m_pMessagesFrame->setFixedWidth((4 * geng.wtile * scale) + 10);
	m_pInfoFrame->setFixedWidth((4 * geng.wtile * scale) + 10);
}

void setplaypausebutton(int paused)
{
	g_pMainWnd->SetPlayPauseButton(paused);
}

void TileWorldMainWnd::SetPlayPauseButton(int paused)
{
	if(paused) {
		m_pBtnPlay->setIcon(playIcon);
	} else {
		m_pBtnPlay->setIcon(pauseIcon);
	}
}

void TileWorldMainWnd::ChangeVolume(int volume)
{
	if(volTimer->isActive()) volTimer->stop();

	m_pPrgVolFrame->setVisible(true);
	m_pPrgVolume->setValue(changevolume(volume));

	connect(volTimer, SIGNAL(timeout()), this, SLOT(HideVolumeWidget()));

    volTimer->setSingleShot(true);
    volTimer->start(2000);
}

void TileWorldMainWnd::HideVolumeWidget()
{
	m_pPrgVolFrame->setVisible(false);
}
