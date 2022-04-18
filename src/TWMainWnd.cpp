/* Copyright (C) 2001-2019 by Madhav Shanbhag, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QTextDocument>
#include <QSortFilterProxyModel>
#include <QFileDialog>
#if defined(Q_OS_WIN)
#include <QStyle>
#endif
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QFontMetrics>
#include <QRect>

#include <cstring>
#include <cmath>

#include "TWMainWnd.h"
#include "TWApp.h"
#include "TWTableSpec.h"
#include "oshwbind.h"
#include "defs.h"
#include "messages.h"
#include "settings.h"
#include "score.h"
#include "state.h"
#include "play.h"
#include "TWMainWnd.h"
#include "fileio.h"
#include "help.h"
#include "timer.h"
#include "sdlsfx.h"
#include "err.h"

using namespace std;

#define CONTINUE_PROPRGATION false
#define STOP_PROPRGATION true

extern bool pedanticmode;

constexpr TileWorldMainWnd::keycmdmap TileWorldMainWnd::keycmds[];

TileWorldMainWnd::TileWorldMainWnd(QWidget* pParent)
	:
	QMainWindow(pParent),
	m_bWindowClosed(false),
	m_pSurface(0),
	m_pInvSurface(0),
	m_bKbdRepeatEnabled(true),
	m_nRuleset(Ruleset_None),
	m_nLevelNum(0),
	m_sLevelName(""),
	m_sLevelPackName(""),
	m_sTimeFormat("%v"),
	m_bProblematic(false),
	m_bOFNT(false),
	m_nBestTime(TIME_NIL),
	m_hintVisible(false),
	m_nTimeLeft(TIME_NIL),
	m_bTimedLevel(false),
	m_bReplay(false),
	m_pSortFilterProxyModel(0)
{
	memset(m_nKeyState, 0, TWK_LAST*sizeof(uint8_t));

	// load scale early so it's there for setupUi
	int percentZoom = getintsetting("zoom");
	if(percentZoom == -1) percentZoom = 100;
	scale = sqrt((double)percentZoom / 100);

	// load ui
	setupUi(this, scale);

	// disable manual window resizing
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	// resdir
	QString appResDir(getdir(RESDIR));

	// load style sheet
	QFile File(appResDir + "/stylesheet.qss");
	File.open(QFile::ReadOnly);
	QString StyleSheet(File.readAll());
	this->setStyleSheet(StyleSheet);

	// initalise blank mouseinfo status before applying event filter
	mouseinfo.state = 0;
	g_pApp->installEventFilter(this);

	connect( m_pTblList, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnListItemActivated(const QModelIndex&)) );
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
	connect( m_pBackButton, SIGNAL(clicked()), this, SLOT(OnBackButton()) );
	connect( m_pImportButton, SIGNAL(clicked()), this, SLOT(OnImportButton()) );

	// change menu to reflect settings
	action_displayCCX->setChecked(getintsetting("displayccx"));
	action_BlurPause->setChecked(getintsetting("blurpause"));
	action_forceShowTimer->setChecked(getintsetting("forceshowtimer") > 0);

	int const tickMS = 1000 / TICKS_PER_SECOND;
	startTimer(tickMS / 2);

	// play pause icon for replay controls
	playIcon = QIcon(appResDir + "/play.svg");
	pauseIcon = QIcon(appResDir + "/pause.svg");

	// show window
	show();

	// timer for display of volume widegt
	volTimer = new QTimer(this);

	// keyboard stuff
	mergeable[CmdNorth] = mergeable[CmdSouth] = CmdWest | CmdEast;
	mergeable[CmdWest] = mergeable[CmdEast] = CmdNorth | CmdSouth;
	SetKeyboardRepeat(true);
}


TileWorldMainWnd::~TileWorldMainWnd()
{
	g_pApp->removeEventFilter(this);

	delete volTimer;
	delete m_pInvSurface;
	delete m_pSurface;
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
	if (isVisible()) {
		switch(pEvent->type()) {
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
				return HandleKeyEvent(pObject, pEvent);
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
				return HandleMouseEvent(pObject, pEvent);
			case QEvent::FocusOut:
				if(action_BlurPause->isChecked())
					PulseKey(TWC_LOSEFOCUS);
				break;
			default: break;
		}
	}

	return CONTINUE_PROPRGATION;
}

bool TileWorldMainWnd::HandleKeyEvent(QObject* pObject, QEvent* pEvent)
{
	// ignore keystrokes when dialogs are active
	if(QApplication::activeModalWidget() != 0) return CONTINUE_PROPRGATION;

	QEvent::Type eType = pEvent->type();

	QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);

	int nQtKey = pKeyEvent->key();

#ifndef NDEBUG
	if(nQtKey < Qt::Key_A || (nQtKey > Qt::Key_Z && nQtKey < Qt::Key_Escape) || nQtKey > Qt::Key_Down)
		return CONTINUE_PROPRGATION;
#else
	if (nQtKey < Qt::Key_Escape || nQtKey > Qt::Key_Down)
		return CONTINUE_PROPRGATION;
#endif

	int nTWKey = -1;
	switch (nQtKey) {
		case Qt::Key_Return:
		case Qt::Key_Enter:  nTWKey = TWK_RETURN; break;
		case Qt::Key_Escape: nTWKey = TWK_ESCAPE; break;
		case Qt::Key_Up:     nTWKey = TWK_UP;     break;
		case Qt::Key_Left:   nTWKey = TWK_LEFT;   break;
		case Qt::Key_Down:   nTWKey = TWK_DOWN;   break;
		case Qt::Key_Right:  nTWKey = TWK_RIGHT;  break;
#ifndef NDEBUG
		case Qt::Key_D:      nTWKey = TWK_DEBUG1; break;
		case Qt::Key_E:      nTWKey = TWK_DEBUG2; break;

		case Qt::Key_C:      nTWKey = TWK_CHIP;   break;
		case Qt::Key_R:      nTWKey = TWK_RED;    break;
		case Qt::Key_B:      nTWKey = TWK_BLUE;   break;
		case Qt::Key_Y:      nTWKey = TWK_YELLOW; break;
		case Qt::Key_G:      nTWKey = TWK_GREEN;  break;

		case Qt::Key_I:      nTWKey = TWK_ICE;    break;
		case Qt::Key_S:      nTWKey = TWK_SLIDE;  break;
		case Qt::Key_F:      nTWKey = TWK_FIRE;   break;
		case Qt::Key_W:      nTWKey = TWK_WATER;  break;
#endif
		default: return CONTINUE_PROPRGATION;
	}

#ifndef NDEBUG
	if(pKeyEvent->modifiers() & Qt::ShiftModifier && nTWKey < 5)
		nTWKey += 4;
#endif

	// record key state
	bool bPress = (eType == QEvent::KeyPress);

	// List view
	QObjectList const & tableWidgets = m_pTablePage->children();
	if (bPress && tableWidgets.contains(pObject)) {
		int currentrow = m_pTblList->selectionModel()->currentIndex().row();
		if ((nTWKey == TWK_RETURN) && currentrow >= 0) {
			g_pApp->exit(CmdProceed);
			return STOP_PROPRGATION;
		} else if(nTWKey == TWK_ESCAPE) {
			g_pApp->exit(CmdQuitLevel);
			return STOP_PROPRGATION;
		} else {
			return CONTINUE_PROPRGATION;
		}
	}

	// Text view
	if(m_pMainWidget->currentIndex() == PAGE_TEXT) {
		if (nTWKey == TWK_RETURN) g_pApp->exit(+1);
		else if(nTWKey == TWK_ESCAPE) g_pApp->exit(CmdQuitLevel);
		return STOP_PROPRGATION;
	}

	if (m_bKbdRepeatEnabled || !pKeyEvent->isAutoRepeat()) {
		KeyEventCallback(nTWKey, bPress);
	}

	// Stop propagating events when the PAGE_GAME is active
	return (m_pMainWidget->currentIndex() == PAGE_GAME);
}


/* This callback is called whenever there is a state change in the
 * mouse buttons. Up events are ignored. Down events are stored to
 * be examined later.
 */
bool TileWorldMainWnd::HandleMouseEvent(QObject* pObject, QEvent* pEvent)
{
	if(pObject != m_pGameWidget) return CONTINUE_PROPRGATION;

	if(pEvent->type() == QEvent::MouseButtonPress) {
		QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);

		mouseinfo.state = KS_PRESSED;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		mouseinfo.x = pMouseEvent->position().x();
		mouseinfo.y = pMouseEvent->position().y();
#else
		mouseinfo.x = pMouseEvent->x();
		mouseinfo.y = pMouseEvent->y();
#endif
		mouseinfo.button = pMouseEvent->button();
	}

	return STOP_PROPRGATION;
}


void TileWorldMainWnd::PulseKey(int nTWKey)
{
	KeyEventCallback(nTWKey, true);
	KeyEventCallback(nTWKey, false);
}


void TileWorldMainWnd::OnPlayback()
{
	int nTWKey = m_bReplay ? TWC_PAUSEGAME : TWC_PLAYBACK;
	PulseKey(nTWKey);
}

void TileWorldMainWnd::OnBackButton()
{
	g_pApp->exit(CmdQuitLevel);
}

void TileWorldMainWnd::OnImportButton()
{
	QFileDialog dialog(this);
	dialog.setDirectory(QDir::homePath());
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter("Levelset Files (*.dat *.ccx)");
	dialog.setLabelText(QFileDialog::Accept, "Import Levelset Files");

	if (!dialog.exec())
		return;

	QStringList fileNames = dialog.selectedFiles();
	if (fileNames.length() == 0)
		return;

	QDir dataDir;
	dataDir.setPath(getdir(USER_SERIESDATDIR));

	QStringList errors;
	foreach (const QString &src, fileNames) {
		// Check file exists
		if(!QFile::exists(src)) {
			errors.append(src + ": file not found");
			continue;
		}

		// Avoid overwriting existing files
		QString fn = QFileInfo(src).fileName();
		if(dataDir.exists(fn)) {
			errors.append(fn + ": a levelpack file with this name already exists");
			continue;
		}

		QString dest = dataDir.filePath(fn);
		if(!QFile::copy(src, dest))
			errors.append(fn + ": failed to copy, read/write error");
	}

	int success = fileNames.length() - errors.length();
	if(errors.length() > 0) {
		if(errors.length() > 1)
			errors.prepend(QString("Copied %1 of %2 files").arg(success).arg(fileNames.length()));

		QMessageBox::warning(this, "Import Failure", errors.join("\n"));
	}

	if (success > 0) {
		g_pApp->exit(CmdReloadLevelsets);
	}
}

/*
 * Keyboard input functions.
 */

/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
void TileWorldMainWnd::SetKeyboardRepeat(bool bEnable)
{
	m_bKbdRepeatEnabled = bEnable;
}


/* Mark all keys as being unpressed at the end of each level
 * as sometimes the program misses a key being released.
 */
void TileWorldMainWnd::ReleaseAllKeys()
{
	for (int k = 0; k < TWK_LAST; ++k) {
		m_nKeyState[k] = false;
		keystates[k] = KS_OFF;
	}
}


/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
void TileWorldMainWnd::CreateGameDisplay()
{
	delete m_pSurface;
	delete m_pInvSurface;

	int w = NXTILES*geng.wtile, h = NYTILES*geng.htile;
	m_pSurface = new Qt_Surface(w, h, false);
	m_pInvSurface = new Qt_Surface(4*geng.wtile, 2*geng.htile, false);

	// this sets the game and objects box
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	geng.screen = m_pSurface;
	m_disploc = TW_Rect(0, 0, w, h);

	SetCurrentPage(PAGE_GAME);

	m_pControlsFrame->setVisible(true);
}


void TileWorldMainWnd::SetCurrentPage(Page ePage)
{
	m_pMainWidget->setCurrentIndex(ePage);
}


/* Fill the display with the background color.
 */
void TileWorldMainWnd::ClearDisplay()
{
	// TODO?
	geng.mapvieworigin = -1;
}


/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
void TileWorldMainWnd::DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime)
{
	bool const bTimedLevel = (pState->game->time > 0);

	m_nTimeLeft = nTimeLeft;

	bool const bForceShowTimer = action_forceShowTimer->isChecked();

	bool bParBad = (pState->game->sgflags & SGF_REPLACEABLE) != 0;

	if (pState->currenttime == -1) {
		// set properties
		m_nRuleset = pState->ruleset;
		m_nLevelNum = pState->game->number;
		m_sLevelName = pState->game->name;
		m_bTimedLevel = bTimedLevel;
		m_bProblematic = false;
		m_nBestTime = nBestTime;
		m_bReplay = false;	// IMPORTANT for OnSpeedValueChanged
		SetSpeed(0);	// IMPORTANT

		// gui stuff
		m_pGameWidget->setCursor(m_nRuleset==Ruleset_MS ? Qt::CrossCursor : Qt::ArrowCursor);
		m_pLCDNumber->display(pState->game->number);
		m_pLblTitle->setText(m_sLevelPackName + " - " + m_sLevelName);
		m_pLblPassword->setText(pState->game->passwd);
		m_pSldSeek->setValue(0);
		action_Pause->setText("Start");

		// easter egg
		m_bOFNT = (m_sLevelName.toUpper() == "YOU CAN'T TEACH AN OLD FROG NEW TRICKS");

		// show/hide controls pane
		bool bHasSolution = (hassolution(pState->game) && ((pState->game->sgflags & SGF_REPLACEABLE) == 0));
		bool bHasDeletedSolution = (hassolution(pState->game) && ((pState->game->sgflags & SGF_REPLACEABLE) != 0));
		m_pControlsFrame->setVisible(bHasSolution);

		// disable/enable menus
		action_Scores->setEnabled(true);
		action_SolutionFiles->setEnabled(true);
		action_TimesClipboard->setEnabled(true);
		action_Levelsets->setEnabled(true);
		action_About->setEnabled(true);
		action_GoTo->setEnabled(true);
		action_Playback->setEnabled(bHasSolution);
		action_Verify->setEnabled(bHasSolution);
		action_Delete->setEnabled(hassolution(pState->game));

		// pedantic mode
		action_PedanticMode->setVisible(m_nRuleset == Ruleset_Lynx);
		action_PedanticMode->setEnabled(m_nRuleset == Ruleset_Lynx);

		// Change delete menu option as appropriate
		if(bHasDeletedSolution) action_Delete->setText("Undelete");
		else action_Delete->setText("Delete");

		// pro- and epilogue
		CCX::Level const & currLevel(m_ccxLevelset.vecLevels[m_nLevelNum]);
		bool hasPrologue(!currLevel.txtPrologue.vecPages.empty());
		bool hasEpilogue(!currLevel.txtEpilogue.vecPages.empty());
		action_Prologue->setEnabled(hasPrologue);
		action_Epilogue->setEnabled(hasEpilogue && bHasSolution);

		// time
		m_pPrgTime->setPar(nBestTime == TIME_NIL ? -1 : nBestTime);
		m_pPrgTime->setParBad(bParBad);

		// set time formatting
		if (bTimedLevel) {
			if (bParBad || nBestTime == TIME_NIL) {
				m_pPrgTime->setFormat("%v");
				m_sTimeFormat  = "%v";
			} else {
				m_pPrgTime->setFormat("%b / %v");
				m_sTimeFormat  = "%v (%d)";
			}
			m_pPrgTime->setFullBar(false);
		} else if(bForceShowTimer) {
			if (bParBad || nBestTime == TIME_NIL) {
				m_pPrgTime->setFormat("[%v]");
				m_sTimeFormat  = "[%v]";
			} else {
				m_pPrgTime->setFormat("[%b] / [%v]");
				m_sTimeFormat  = "[%v] (%d)";
			}
			m_pPrgTime->setFullBar(false);
		} else {
			m_pPrgTime->setFormat("---");
			m_sTimeFormat  = "---";
			m_pPrgTime->setFullBar(true);
		}

		// set time limits
		int timeLimit = bTimedLevel ? pState->game->time : 999;
		if (nBestTime != TIME_NIL) {
			m_pSldSeek->setMaximum(timeLimit - nBestTime);
		}
		m_pPrgTime->setMaximum(timeLimit);
		m_pPrgTime->setValue(timeLimit);

		// Hide hint and set text
		SetHintVisibility(false);
		SetHintText(pState->hinttext);

		// This sets m_bProblematic as true if there are any problems
		CheckForProblems(pState);

		Narrate(&CCX::Level::txtPrologue);
	}
	// do these on play start - they only need to be done once
	else if(action_Levelsets->isEnabled()) {
		m_bReplay = (pState->replay >= 0);
		m_pControlsFrame->setVisible(m_bReplay);
		if (m_bProblematic) {
			SetHintVisibility(false);
			m_bProblematic = false;
		}

		// disable menus
		action_Scores->setEnabled(false);
		action_SolutionFiles->setEnabled(false);
		action_TimesClipboard->setEnabled(false);
		action_Levelsets->setEnabled(false);
		action_Playback->setEnabled(false);
		action_Verify->setEnabled(false);
		action_Delete->setEnabled(false);
		action_About->setEnabled(false);
		action_GoTo->setEnabled(false);
		action_Prologue->setEnabled(false);
		action_Epilogue->setEnabled(false);
		action_PedanticMode->setEnabled(false);

		m_pPrgTime->setFormat(m_sTimeFormat);
	}

	// display blank pause screen in ms mode
	if (pState->statusflags & SF_SHUTTERED) {
		DisplayShutter();
	} else {
		DisplayMapView(pState);
	}

	// draw objects widget
	for (int i = 0; i < 4; ++i) {
		drawfulltileid(m_pInvSurface, i*geng.wtile, 0,
			(pState->keys[i] ? Key_Red+i : Empty));
		drawfulltileid(m_pInvSurface, i*geng.wtile, geng.htile,
			(pState->boots[i] ? Boots_Ice+i : Empty));
	}
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	// chips left
	m_pLCDChipsLeft->display(pState->chipsneeded);

	// time left
	m_pPrgTime->setValue(nTimeLeft);

	// move progress slider in replay mode
	if (m_bReplay && !m_pSldSeek->isSliderDown()) {
		m_pSldSeek->blockSignals(true);
		m_pSldSeek->setValue(pState->currenttime / TICKS_PER_SECOND);
		m_pSldSeek->blockSignals(false);
	}

	// set the hint when on a relevant tile
	if (!m_bProblematic) {
		// Call setText / clear only when really required
		// See comments about QLabel in TWDisplayWidget.h
		if ((pState->statusflags & SF_SHOWHINT) != 0) {
			SetHintVisibility(true);
		} else {
			SetHintVisibility(false);
		}
	}
}

void TileWorldMainWnd::CheckForProblems(const gamestate* pState)
{
	QString s;

	if (pState->statusflags & SF_INVALID) {
		s = "This level cannot be played.";
	} else if (pState->game->unsolvable) {
		s = "This level is reported to be unsolvable";
		if (*pState->game->unsolvable)
			s += ": " + QString(pState->game->unsolvable);
		s += ".";
	} else {
		CCX::RulesetCompatibility ruleCompat = m_ccxLevelset.vecLevels[m_nLevelNum].ruleCompat;
		CCX::Compatibility compat = CCX::COMPAT_UNKNOWN;
		if (m_nRuleset == Ruleset_Lynx) {
			if (pedanticmode)
				compat = ruleCompat.ePedantic;
			else
				compat = ruleCompat.eLynx;
		} else if (m_nRuleset == Ruleset_MS) {
			compat = ruleCompat.eMS;
		}

		if (compat == CCX::COMPAT_NO){
			s = "This level is flagged as being incompatible with the current ruleset.";
		}
	}

	m_bProblematic = !s.isEmpty();
	if (m_bProblematic) {
		SetHintText(s);
		SetHintVisibility(true);
	}
}

void TileWorldMainWnd::DisplayMapView(const gamestate* pState)
{
	short xviewpos = pState->xviewpos;
	short yviewpos = pState->yviewpos;
	bool bFrogShow = (m_bOFNT  &&  m_bReplay  &&
					xviewpos/8 == 14  &&  yviewpos/8 == 9);
	if (bFrogShow) {
		int x = xviewpos, y = yviewpos;
		if (m_nRuleset == Ruleset_MS) {
			for (int pos = 0; pos < CXGRID*CYGRID; ++pos) {
				int id = pState->map[pos].top.id;
				if ( ! (id >= Teeth && id < Teeth+4) )
					continue;
				x = (pos % CXGRID) * 8;
				y = (pos / CXGRID) * 8;
				break;
			}
		} else {
			for (const creature* p = pState->creatures; p->id != 0; ++p) {
				if ( ! (p->id >= Teeth && p->id < Teeth+4) )
					continue;
				x = (p->pos % CXGRID) * 8;
				y = (p->pos / CXGRID) * 8;
				if (p->moving > 0) {
					switch (p->dir) {
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

	if (bFrogShow) {
		const_cast<gamestate*>(pState)->xviewpos = xviewpos;
		const_cast<gamestate*>(pState)->yviewpos = yviewpos;
	}
}

void TileWorldMainWnd::DisplayShutter()
{
	QPixmap pixmap(NXTILES*geng.wtile, NYTILES*geng.htile);
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
int TileWorldMainWnd::DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted)
{
	if (nCompleted == 0)
		return CmdNone;

	if (nCompleted == -2)	// abandoned
		return CmdNone;

	QMessageBox msgBox(this);

	if (nCompleted > 0)	 { // Success
		QString sText;
		QTextStream strm(&sText);
		strm.setLocale(m_locale);
		strm << "<big><b>" << m_sLevelName << "</b></big><br>";

		QString sAuthor = m_ccxLevelset.vecLevels[m_nLevelNum].sAuthor;
		if (!sAuthor.isEmpty())
			strm << "by " << sAuthor;

		strm << "<hr><br><big><b>";
		if (m_bReplay) {
			strm << "Alright!";
		} else {
			strm << getmessage(MessageWin, "You won!");
		}
		strm << "</b></big><br>";

		if (!m_bReplay) {
			if (m_bTimedLevel && m_nBestTime != TIME_NIL) {
				int diff = m_nTimeLeft - m_nBestTime;

				if (diff == 0)
					strm << "You scored " << m_nBestTime << " yet again.";
				else if (diff == 1)
					strm << "You made it 1 second faster this time!";
				else if (diff > 0)
					strm << "You made it " << diff << " seconds faster this time!";
				else
					strm << "But not as quick as your previous score of " << m_nBestTime << "...";
			}

			strm << "<br><table width='100%'>"
			<< "<tr><td>Time Bonus:</td><td align='right'>"  << nTimeScore << "</td></tr>"
			<< "<tr><td>Level Bonus:</td><td align='right'>" << nBaseScore << "</td></tr>"
			<< "<tr><td>Level Score:</td><td align='right'>" << (nTimeScore + nBaseScore) << "</td></tr>"
			<< "<tr><td colspan='2'><hr></td></tr>"
			<< "<tr><td>Total Score:</td><td align='right'>" << lTotalScore << "</td></tr>"
			<< "</table>";
		}

		msgBox.setTextFormat(Qt::RichText);
		msgBox.setText(sText);

		Qt_Surface* pSurface = new Qt_Surface(geng.wtile, geng.htile, false);
		drawfulltileid(pSurface, 0, 0, Exited_Chip);
		msgBox.setIconPixmap(pSurface->GetPixmap());
		delete pSurface;

		msgBox.setWindowTitle(m_bReplay ? "Replay Completed" : "Level Completed");

		m_sTextToCopy = timestring(m_nLevelNum, m_sLevelName, m_nTimeLeft, m_bTimedLevel, false);

		msgBox.addButton("&Onward!", QMessageBox::AcceptRole);
		QPushButton* pBtnRestart = msgBox.addButton("&Restart", QMessageBox::AcceptRole);
		QPushButton* pBtnCopyScore = msgBox.addButton("&Copy Score", QMessageBox::ActionRole);
		connect( pBtnCopyScore, SIGNAL(clicked()), this, SLOT(OnCopyText()) );

		msgBox.exec();
		ReleaseAllKeys();
		if (msgBox.clickedButton() == pBtnRestart)
			return CmdSameLevel;

		Narrate(&CCX::Level::txtEpilogue);
	} else {	// Failure
		bool bTimeout = (m_bTimedLevel  &&  m_nTimeLeft <= 0);
		if (m_bReplay) {
			QString sMsg = "Whoa! Chip ";
			if (bTimeout)
				sMsg += "ran out of time";
			else
				sMsg += "ran into some trouble";
			// TODO: What about when Chip just doesn't reach the exit or reaches the exit too early?
			sMsg += " there.\nIt looks like the level has changed after that solution was recorded.";
			msgBox.setText(sMsg);
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle("Replay Failed");
		} else {
			QString szMsg;
			if (bTimeout) {
				szMsg = getmessage(MessageTime, "You ran out of time.");
			} else {
				szMsg = getmessage(MessageDie, "You died.");
			}

			msgBox.setTextFormat(Qt::PlainText);
			msgBox.setText(szMsg);
			// On Windows, using setIcon with QMessageBox::Warning causes the corresponding
			// system sound to play. Using setIconPixmap avoids this. But avoid doing this
			// on Linux as it can produce a style warning.
			#if defined(Q_OS_WIN)
				QStyle* pStyle = g_pApp->style();
				if (pStyle != 0) {
					QIcon icon = pStyle->standardIcon(QStyle::SP_MessageBoxWarning);
					msgBox.setIconPixmap(icon.pixmap(48));
				}
			#else
				msgBox.setIcon(QMessageBox::Warning);
			#endif
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
int TileWorldMainWnd::DisplayList(TWTableSpec* table, int* pnIndex,
		bool showRulesetOptions)
{
	int nCmd = 0;

	// disable menus
	bool menuStatus[] = {
		action_Scores->isEnabled(),
		action_SolutionFiles->isEnabled(),
		action_TimesClipboard->isEnabled(),
		action_Levelsets->isEnabled(),
		menu_Level->isEnabled(),
		menu_Solution->isEnabled(),
		menu_Options->isEnabled(),
		menu_Zoom->isEnabled(),
		menu_Solution->isEnabled()
	};

	action_Scores->setEnabled(false);
	action_SolutionFiles->setEnabled(false);
	action_TimesClipboard->setEnabled(false);
	action_Levelsets->setEnabled(false);
	menu_Level->setEnabled(false);
	menu_Solution->setEnabled(false);
	menu_Options->setEnabled(false);
	menu_Zoom->setEnabled(false);
	menu_Solution->setEnabled(false);

	// dummy scope to force table spec destructors before ExitTWorld
	{
		table->fixRows();
		QSortFilterProxyModel proxyModel;
		m_pSortFilterProxyModel = &proxyModel;
		proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
		proxyModel.setFilterKeyColumn(-1);
		proxyModel.setSourceModel(table);
		m_pTblList->setModel(&proxyModel);

		m_pTblList->horizontalHeader()->setStretchLastSection(table->cols() == 1);

		QModelIndex index = proxyModel.mapFromSource(table->index(*pnIndex, 0));
		m_pTblList->setCurrentIndex(index);
		m_pTblList->resizeColumnsToContents();
		m_pTblList->resizeRowsToContents();
		m_pTxtFind->clear();
		SetCurrentPage(PAGE_TABLE);
		m_pTblList->setFocus();

		m_pComboRuleset->setVisible(showRulesetOptions);
		m_pComboLabel->setVisible(showRulesetOptions);
		m_pImportButton->setVisible(showRulesetOptions);

		nCmd = g_pApp->exec();

		*pnIndex = proxyModel.mapToSource(m_pTblList->currentIndex()).row();

		SetCurrentPage(PAGE_GAME);
		m_pTblList->setModel(0);
		m_pSortFilterProxyModel = 0;
	}

	if (m_bWindowClosed) g_pApp->ExitTWorld();

	// reenable menus
	action_Scores->setEnabled(menuStatus[0]);
	action_SolutionFiles->setEnabled(menuStatus[1]);
	action_TimesClipboard->setEnabled(menuStatus[2]);
	action_Levelsets->setEnabled(menuStatus[3]);
	menu_Level->setEnabled(menuStatus[4]);
	menu_Solution->setEnabled(menuStatus[5]);
	menu_Options->setEnabled(menuStatus[6]);
	menu_Zoom->setEnabled(menuStatus[7]);
	menu_Solution->setEnabled(menuStatus[8]);

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
	if (n == 0) {
		TileWorldApp::Bell();
		return;
	}

	m_pTblList->setFocus();

	if (!m_pTblList->currentIndex().isValid())
		m_pTblList->selectRow(0);

	if (n == 1)
		g_pApp->exit(CmdProceed);
}

/* Display an input prompt to the user. prompt supplies the prompt to
 * display.
 */
bool TileWorldMainWnd::DisplayYesNoPrompt(const char* prompt)
{
	QMessageBox::StandardButton eBtn = QMessageBox::question(
		this, TileWorldApp::applicationName(), prompt, QMessageBox::Yes|QMessageBox::No);
	return eBtn == QMessageBox::Yes;
}


const char *TileWorldMainWnd::DisplayPasswordPrompt()
{
	QString password = QInputDialog::getText(this, TileWorldApp::applicationName(), "Enter Password");
	if (password.isEmpty()) return "";
	password.truncate(4);
	password = password.toUpper();
	return password.toUtf8().constData();
}

/*
 * The subtitle stack
 */
void TileWorldMainWnd::PushSubtitle(QString subtitle)
{
	subtitlestack.append(subtitle);
	SetSubtitle(subtitle);
}

void TileWorldMainWnd::PopSubtitle()
{
	if(!subtitlestack.isEmpty()) {
		subtitlestack.removeLast();
	}

	if(!subtitlestack.isEmpty()) {
		SetSubtitle(subtitlestack.last());
	} else {
		SetSubtitle("");
	}
}

void TileWorldMainWnd::ChangeSubtitle(QString subtitle)
{
	if(!subtitlestack.isEmpty()) {
		subtitlestack.last() = subtitle;
	}
	SetSubtitle(subtitle);
}


/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
void TileWorldMainWnd::SetSubtitle(QString subtitle)
{
	QString sTitle = TileWorldApp::applicationName();
	if (!subtitle.isEmpty())
		sTitle += " - " + subtitle;
	setWindowTitle(sTitle);
}

int TileWorldMainWnd::GetSelectedRuleset()
{
	return m_pComboRuleset->currentText() == "MS" ? Ruleset_MS : Ruleset_Lynx;
}
void TileWorldMainWnd::SetSelectedRuleset(int r)
{
	if(r == Ruleset_MS)
		m_pComboRuleset->setCurrentText("MS");
	else if(r == Ruleset_Lynx)
		m_pComboRuleset->setCurrentText("Lynx");
}

/* Read any additional data for the series.
 */
void TileWorldMainWnd::ReadExtensions(gameseries* pSeries)
{
	QDir dataDir;
	dataDir.setPath(getdir(pSeries->mapfiledir));

	QString sSetName = QFileInfo(pSeries->mapfilename).completeBaseName();
	m_sLevelPackName = sSetName; // save for use on display

	QString sFilePath = dataDir.filePath(sSetName + ".ccx");

	m_ccxLevelset.Clear();
	if (!m_ccxLevelset.ReadFile(sFilePath, pSeries->count))
		warn("%s: failed to read file", sFilePath.toUtf8().constData());

	for (int i = 1; i <= pSeries->count; ++i) {
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
	for (int nPage = 0; nPage < n; nPage += d) {
		m_pBtnTextPrev->setVisible(nPage > 0);

		CCX::Page& rPage = rText.vecPages[nPage];

		QTextDocument* pDoc = m_pTextBrowser->document();
		if (pDoc != 0) {
			if (!m_ccxLevelset.sStyleSheet.isEmpty())
				pDoc->setDefaultStyleSheet(m_ccxLevelset.sStyleSheet);
			pDoc->setDocumentMargin(16);
		}

		QString sText = rPage.sText;
		if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN) {
			m_pTextBrowser->setPlainText(sText);
		} else {
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
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setWindowTitle("About Tile World");
	msgBox->setTextFormat(Qt::RichText);
	msgBox->setText(aboutText);
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setAttribute(Qt::WA_DeleteOnClose);
	msgBox->exec();
	delete msgBox;
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
	if (pAction == action_Prologue) {
		Narrate(&CCX::Level::txtPrologue, true);
		return;
	}

	if (pAction == action_Epilogue) {
		Narrate(&CCX::Level::txtEpilogue, true);
		return;
	}

	if (pAction == action_displayCCX) {
		setintsetting("displayccx", pAction->isChecked() ? 1 : 0);
		return;
	}

	if (pAction == action_BlurPause) {
		setintsetting("blurpause", pAction->isChecked() ? 1 : 0);
		return;
	}

	if (pAction == action_forceShowTimer) {
		setintsetting("forceshowtimer", pAction->isChecked() ? 1 : 0);
		drawscreen(true);
		return;
	}

	if (pAction == action_About) {
		ShowAbout();
		return;
	}

	if (pAction->actionGroup()  == actiongroup_Zoom) {
		int s = getintsetting("zoom");
		if(s == -1) s = 100;

		if(pAction == action_Zoom100) {
		    s = 100;
		} else if(pAction == action_ZoomMinus) {
		    s -= 20;
		} else { // action_ZoomPlus
		    s += 20;
		}

		setintsetting("zoom", s);
		this->SetScale(s);
		return;
	}

	if (pAction == action_VolumeUp) {
		this->ChangeVolume(+1);
		return;
	}

	if (pAction == action_VolumeDown) {
		this->ChangeVolume(-1);
		return;
	}

	if (pAction == action_Step) {
		// step dialog
		QInputDialog stepDialog(this);
		stepDialog.setWindowTitle("Step");
		stepDialog.setLabelText("Set level step value");

		if (GetSelectedRuleset() == Ruleset_Lynx) {
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

	if (pAction == action_PedanticMode) {
		setpedanticmode(pAction->isChecked());
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

	if (pAction == action_Pause) return TWC_PAUSEGAME;
	if (pAction == action_Restart) return TWC_SAMELEVEL;
	if (pAction == action_Next) return TWC_NEXTLEVEL;
	if (pAction == action_Previous) return TWC_PREVLEVEL;
	if (pAction == action_GoTo) return TWC_GOTOLEVEL;

	if (pAction == action_Playback) return TWC_PLAYBACK;
	if (pAction == action_Verify) return TWC_CHECKSOLUTION;
	if (pAction == action_Delete) return TWC_DELSOLUTION;

	return TWK_dummy;
}

void TileWorldMainWnd::ResizeHintFont()
{
	SetHintText(m_pLblHint->text());
}

void TileWorldMainWnd::SetHintText(QString hint)
{
	// Calculate available dimensions for hint
	int availableHeight = m_pLblTitle->geometry().bottom() - m_pObjectsContainer->geometry().y();
	int availableWidth = m_pInfoFrame->width();
	int margins = (m_pLblHint->margin() + m_pMessagesFrame->frameWidth()) * 2;
	availableHeight -= margins;
	availableWidth -= margins;

	// decrease font size
	QFont thisFont = m_pLblHint->font();
	for(int fs=25; fs > 12; fs--) {
		thisFont.setPixelSize(fs);
		QFontMetrics fm(thisFont);
		QRect r = fm.boundingRect(0, 0, availableWidth, 1000, Qt::TextWordWrap, hint);

		if(r.height() <= availableHeight) break;
	}

	m_pLblHint->setFont(thisFont);
	m_pLblHint->setText(hint);
}

void TileWorldMainWnd::SetHintVisibility(bool newmode)
{
	bool changed = (newmode != m_hintVisible);
	m_hintVisible = newmode;

	if(!changed) {
		return;
	} else if(m_hintVisible) {
		m_pInfoPane->setCurrentIndex(1);
	} else {
		m_pInfoPane->setCurrentIndex(0);
	}
}

void TileWorldMainWnd::SetScale(int s, bool checkPrevScale)
{
	double newScale = (double)s / 100;
	if(checkPrevScale && newScale == scale) return;

	// set the property
	scale = sqrt(newScale);

	if(m_pSurface == 0 || m_pInvSurface == 0 || geng.wtile < 1) {
		warn("Attempt to set pixmap and scale without setting pixmap first");
		return;
	}

	// Hide the hint to avoid knock-on layout issues
	if(m_hintVisible == true) {
		SetHintVisibility(false);
	}

	// align widget size to specified scale
	m_pGameWidget->setFixedSize(scale * DEFAULTTILE * NXTILES, scale * DEFAULTTILE * NYTILES);
	m_pObjectsWidget->setFixedSize(scale * DEFAULTTILE * 4, scale * DEFAULTTILE * 2);

	// this sets the game and objects box
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	// this aligns the width of the objects box with the other elements
	// in the right column
	m_pMessagesFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);
	m_pInfoFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);

	// we need to determine hint font size again
	ResizeHintFont();
}

void TileWorldMainWnd::SetPlayPauseButton(bool paused)
{
	if(paused) {
		m_pBtnPlay->setIcon(playIcon);
		action_Pause->setText("Resume");
	} else {
		m_pBtnPlay->setIcon(pauseIcon);
		action_Pause->setText("Pause");
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


/* This callback is called whenever the state of any keyboard key
 * changes. It records this change in the keystates array. The key can
 * be recorded as being struck, pressed, repeating, held down, or down
 * but ignored, as appropriate to when they were first pressed and the
 * current behavior settings. Shift-type keys are always either on or
 * off.
 */
void TileWorldMainWnd::KeyEventCallback(int scancode, bool down)
{
	if (down) {
		keystates[scancode] = keystates[scancode] == KS_OFF ? KS_PRESSED : KS_REPEATING;
	} else {
		keystates[scancode] = keystates[scancode] == KS_PRESSED ? KS_STRUCK : KS_OFF;
	}
}

/* Initialize (or re-initialize) all key states.
 */
void TileWorldMainWnd::RestartKeystates(void)
{
	memset(keystates, KS_OFF, sizeof keystates);
	for (int n = 0; n < TWK_LAST; ++n)
	if (m_nKeyState[n])
		KeyEventCallback(n, true);
}

/* Update the key states. This is done at the start of each polling
 * cycle. The state changes that occur depend on the current behavior
 * settings.
 */
void TileWorldMainWnd::ResetKeyStates(void)
{
	for (int n = 0 ; n < TWK_LAST ; ++n) {
		int x = (int)keystates[n];

		if(x == KS_STRUCK) {
			keystates[n] = KS_OFF;
		} else if(x == KS_DOWNBUTOFF2 || x == KS_DOWNBUTOFF3 || x == KS_REPEATING) {
			keystates[n] = KS_DOWN;
		} else if(x == KS_PRESSED) {
			keystates[n] = joystickstyle ? KS_DOWN : KS_DOWNBUTOFF1;
		} else if(x == KS_DOWNBUTOFF1) {
			keystates[n] = joystickstyle ? KS_DOWN : KS_DOWNBUTOFF2;
		}
	}
}

/*
 * Mouse event functions.
 */

/* Given a pixel's coordinates, return the integer identifying the
 * tile's position in the map, or -1 if the pixel is not on the map view.
 */
int TileWorldMainWnd::WindowMapPos(int x, int y)
{
	if (geng.mapvieworigin < 0)
		return -1;
	if (x < 0 || y < 0)
		return -1;

	double t = DEFAULTTILE * scale;
	x *= 4 / t;
	y *= 4 / t;

	if (x >= NXTILES * 4 || y >= NYTILES * 4)
		return -1;

	x = (x + geng.mapvieworigin % (CXGRID * 4)) / 4;
	y = (y + geng.mapvieworigin / (CXGRID * 4)) / 4;

	if (x < 0 || x >= CXGRID || y < 0 || y >= CYGRID) {
		warn("mouse moved off the map: (%d %d)", x, y);
		return -1;
	}

	return y * CXGRID + x;
}

/* Return the command appropriate to the most recent mouse activity.
 */
int TileWorldMainWnd::RetrieveMouseCommand(void)
{
	switch (mouseinfo.state) {
		case KS_PRESSED:
			mouseinfo.state = KS_OFF;
			if (mouseinfo.button == Qt::LeftButton) {
				int n = WindowMapPos(mouseinfo.x, mouseinfo.y);
				if (n >= 0) {
					mouseinfo.state = KS_DOWNBUTOFF1;
					return CmdAbsMouseMoveFirst + n;
				}
			}
			break;
		case KS_DOWNBUTOFF1:
			mouseinfo.state = KS_DOWNBUTOFF2;
			return CmdPreserve;
		case KS_DOWNBUTOFF2:
			mouseinfo.state = KS_DOWNBUTOFF3;
			return CmdPreserve;
		case KS_DOWNBUTOFF3:
			mouseinfo.state = KS_OFF;
			return CmdPreserve;
	}
	return 0;
}

/* Poll the keyboard and return the command associated with the
 * selected key, if any. If no key is selected and wait is TRUE, block
 * until a key with an associated command is selected. In keyboard behavior
 * mode, the function can return CmdPreserve, indicating that if the key
 * command from the previous poll has not been processed, it should still
 * be considered active. If two mergeable keys are selected, the return
 * value will be the bitwise-or of their command values.
 */
int TileWorldMainWnd::Input(bool wait)
{
	keycmdmap const    *kc;
	bool		lingerflag = false;
	int			cmd1, cmd, n;

	for (;;) {
		ResetKeyStates();
		eventupdate(wait);

		cmd1 = cmd = 0;
		for (kc = keycmds ; kc->scancode ; ++kc) {
			n = keystates[kc->scancode];
			if (!n) continue;
			if (n == KS_PRESSED || (kc->hold && n == KS_DOWN)) {
				if (!cmd1) {
					cmd1 = kc->cmd;
					if (!joystickstyle || cmd1 > CmdKeyMoveLast || !mergeable[cmd1])
						return cmd1;
				} else {
					if (cmd1 <= CmdKeyMoveLast && (mergeable[cmd1] & kc->cmd) == kc->cmd)
						return cmd1 | kc->cmd;
				}
			} else if (n == KS_STRUCK || n == KS_REPEATING) {
				cmd = kc->cmd;
			} else if (n == KS_DOWNBUTOFF1 || n == KS_DOWNBUTOFF2) {
				lingerflag = true;
			}
		}
		if (cmd1)
			return cmd1;
		if (cmd)
			return cmd;
		cmd = RetrieveMouseCommand();
		if (cmd)
			return cmd;
		if (!wait)
			break;
	}
	if (!cmd && lingerflag)
		cmd = CmdPreserve;
	return cmd;
}

/* Turn joystick behavior mode on or off. In joystick-behavior mode,
 * the arrow keys are always returned from input() if they are down at
 * the time of the polling cycle. Other keys are only returned if they
 * are pressed during a polling cycle (or if they repeat, if keyboard
 * repeating is on). In keyboard-behavior mode, the arrow keys have a
 * special repeating behavior that is kept synchronized with the
 * polling cycle.
 */
bool TileWorldMainWnd::SetKeyboardArrowsRepeat(bool enable)
{
	joystickstyle = enable;
	RestartKeystates();
	return true;
}
