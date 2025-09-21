/* Copyright (C) 2001-2019 by Madhav Shanbhag, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QtWidgets/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QPushButton>
#include <QtGui/QTextDocument>
#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QFileDialog>
#if defined(Q_OS_WIN)
#include <QtWidgets/QStyle>
#endif
#include <QtGui/QPainter>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QFontMetrics>
#include <QtCore/QRect>
#include <QtGui/QWindow>

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
    m_windowClosed(false),
    m_surface(0),
    m_invSurface(0),
    m_kbdRepeatEnabled(true),
    m_ruleset(Ruleset_None),
    m_levelNum(0),
    m_levelName(""),
    m_levelPackName(""),
    m_timeFormat("%v"),
    m_problematic(false),
    m_oFNT(false),
    m_bestTime(TIME_NIL),
    m_hintVisible(false),
    m_timeLeft(TIME_NIL),
    m_timedLevel(false),
    m_replay(false),
    m_sortFilterProxyModel(0)
{
    memset(m_keyState, 0, TWK_LAST*sizeof(uint8_t));

    // load scale early so it's there for setupUi
    int percentZoom = getintsetting("zoom");
    if(percentZoom == -1) percentZoom = 100;
    m_scale = sqrt((double)percentZoom / 100);

    // load ui
    setupUi(this, m_scale);

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
    m_mouseinfo.state = 0;
    g_app->installEventFilter(this);

    connect( m_tableList, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnListItemActivated()) );
    connect( m_textFind, SIGNAL(textChanged(const QString&)), this, SLOT(OnFindTextChanged(const QString&)) );
    connect( m_textFind, SIGNAL(returnPressed()), this, SLOT(OnFindReturnPressed()) );
    connect( m_buttonPlay, SIGNAL(clicked()), this, SLOT(OnPlayback()) );
    connect( m_slideSpeed, SIGNAL(valueChanged(int)), this, SLOT(OnSpeedValueChanged(int)) );
    connect( m_slideSpeed, SIGNAL(sliderReleased()), this, SLOT(OnSpeedSliderReleased()) );
    connect( m_slideSeek, SIGNAL(valueChanged(int)), this, SLOT(OnSeekPosChanged(int)) );
    connect( m_buttonTextNext, SIGNAL(clicked()), this, SLOT(OnTextNext()) );
    connect( m_buttonTextPrev, SIGNAL(clicked()), this, SLOT(OnTextPrev()) );
    connect( m_buttonTextReturn, SIGNAL(clicked()), this, SLOT(OnTextReturn()) );
    connect( m_menuBar, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionTriggered(QAction*)) );
    connect( m_backButton, SIGNAL(clicked()), this, SLOT(OnBackButton()) );
    connect( m_goButton, SIGNAL(clicked()), this, SLOT(OnListItemActivated()) );
    connect( qGuiApp, SIGNAL(focusWindowChanged(QWindow*)), this, SLOT(FocusChanged(QWindow*))),

    // change menu to reflect settings
    action_displayCCX->setChecked(getintsetting("displayccx"));
    action_BlurPause->setChecked(getintsetting("blurpause"));
    action_forceShowTimer->setChecked(getintsetting("forceshowtimer") > 0);

    int const tickMS = 1000 / TICKS_PER_SECOND;
    startTimer(tickMS / 2);

    // play pause icon for replay controls
    m_playIcon = QIcon(appResDir + "/play.svg");
    m_pauseIcon = QIcon(appResDir + "/pause.svg");

    // validate window size
    layout()->activate();

    // centre on screen
    QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    QRect w(screen.topLeft(), (screen.size() - frameSize()) / 2);
    move(w.bottomRight());

    // show the window
    show();

    // timer for display of volume widegt
    m_volTimer = new QTimer(this);

    // keyboard stuff
    m_mergeable[CmdNorth] = m_mergeable[CmdSouth] = CmdWest | CmdEast;
    m_mergeable[CmdWest] = m_mergeable[CmdEast] = CmdNorth | CmdSouth;
    SetKeyboardRepeat(true);
}


TileWorldMainWnd::~TileWorldMainWnd()
{
    g_app->removeEventFilter(this);

    delete m_volTimer;
    delete m_invSurface;
    delete m_surface;
}


void TileWorldMainWnd::closeEvent(QCloseEvent* pCloseEvent)
{
    QMainWindow::closeEvent(pCloseEvent);
    m_windowClosed = true;

    if (m_mainWidget->currentIndex() == PAGE_GAME)
        g_app->ExitTWorld();
    else
        g_app->quit();
}

bool TileWorldMainWnd::eventFilter(QObject* pObject, QEvent* pEvent)
{
    if (isVisible()) {
        switch(pEvent->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
                return HandleKeyEvent(pObject, static_cast<QKeyEvent*>(pEvent));
            case QEvent::MouseButtonPress:
                if(pObject == m_gameWidget) {
                    HandleMouseEvent(static_cast<QMouseEvent*>(pEvent));
                    return STOP_PROPRGATION;
                }
            case QEvent::MouseButtonRelease:
                if(pObject == m_gameWidget) {
                    return STOP_PROPRGATION;
                }
            default: break;
        }
    }

    return CONTINUE_PROPRGATION;
}

void TileWorldMainWnd::FocusChanged(QWindow *w)
{
    if(!w && action_BlurPause->isChecked())
        PulseKey(CmdLostFocus);
}

bool TileWorldMainWnd::HandleKeyEvent(QObject* pObject, QKeyEvent* pKeyEvent)
{
    // ignore keystrokes when dialogs are active
    if(QApplication::activeModalWidget() != 0) return CONTINUE_PROPRGATION;

    int nTWKey;
    switch(m_mainWidget->currentIndex()) {
        case PAGE_GAME:
            if (!m_kbdRepeatEnabled && pKeyEvent->isAutoRepeat())
                return STOP_PROPRGATION;

            switch (pKeyEvent->key()) {
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
            if(pKeyEvent->modifiers() & Qt::ShiftModifier) {
                switch (nTWKey) {
                    case TWK_UP:     nTWKey = TWK_UP_CHEAT; break;
                    case TWK_LEFT:   nTWKey = TWK_LEFT_CHEAT; break;
                    case TWK_DOWN:   nTWKey = TWK_DOWN_CHEAT; break;
                    case TWK_RIGHT:  nTWKey = TWK_RIGHT_CHEAT; break;
                }
            }
#endif

            // send keystroke
            KeyEventCallback(nTWKey, pKeyEvent->type() == QEvent::KeyPress);
            return STOP_PROPRGATION;

        case PAGE_TABLE:
            if (pKeyEvent->type() != QEvent::KeyPress
                    || !m_tablePage->children().contains(pObject))
                return CONTINUE_PROPRGATION;

            switch (pKeyEvent->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    if (m_tableList->selectionModel()->currentIndex().row() >= 0)
                        g_app->exit(CmdProceed);
                    return STOP_PROPRGATION;
                    break;

                case Qt::Key_Escape:
                    g_app->exit(CmdQuitLevel);
                    return STOP_PROPRGATION;
                    break;
            }

            return CONTINUE_PROPRGATION;

        case PAGE_TEXT:
            switch (pKeyEvent->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    g_app->exit(+1);
                    return STOP_PROPRGATION;
                    break;

                case Qt::Key_Escape:
                    g_app->exit(CmdQuitLevel);
                    return STOP_PROPRGATION;
                    break;
            }

            return CONTINUE_PROPRGATION;

        default:
            return CONTINUE_PROPRGATION;
    }
}


/* This callback is called whenever there is a state change in the
 * mouse buttons. Up events are ignored. Down events are stored to
 * be examined later.
 */
void TileWorldMainWnd::HandleMouseEvent(QMouseEvent* pMouseEvent)
{
    m_mouseinfo.state = KS_PRESSED;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    m_mouseinfo.x = pMouseEvent->position().x();
    m_mouseinfo.y = pMouseEvent->position().y();
#else
    m_mouseinfo.x = pMouseEvent->x();
    m_mouseinfo.y = pMouseEvent->y();
#endif
    m_mouseinfo.button = pMouseEvent->button();
}


void TileWorldMainWnd::PulseKey(int cmd)
{
    m_nextcommand = cmd;
}


void TileWorldMainWnd::OnPlayback()
{
    PulseKey(m_replay ? CmdPauseGame : CmdPlayback);
}

void TileWorldMainWnd::OnBackButton()
{
    g_app->exit(CmdQuitLevel);
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
        g_app->exit(CmdReloadLevelsets);
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
    m_kbdRepeatEnabled = bEnable;
}


/* Mark all keys as being unpressed at the end of each level
 * as sometimes the program misses a key being released.
 */
void TileWorldMainWnd::ReleaseAllKeys()
{
    for (int k = 0; k < TWK_LAST; ++k) {
        m_keyState[k] = false;
        m_keystates[k] = KS_OFF;
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
    delete m_surface;
    delete m_invSurface;

    int w = NXTILES*geng.wtile, h = NYTILES*geng.htile;
    m_surface = new Qt_Surface(w, h, false);
    m_invSurface = new Qt_Surface(4*geng.wtile, 2*geng.htile, false);

    // this sets the game and objects box
    m_gameWidget->setPixmap(m_surface->GetPixmap());
    m_objectsWidget->setPixmap(m_invSurface->GetPixmap());

    geng.screen = m_surface;
    m_disploc = TW_Rect(0, 0, w, h);

    SetCurrentPage(PAGE_GAME);

    m_controlsFrame->setVisible(true);
}


void TileWorldMainWnd::SetCurrentPage(Page ePage)
{
    m_mainWidget->setCurrentIndex(ePage);
}


/* Fill the display with the background color.
 */
void TileWorldMainWnd::ClearDisplay()
{
    // TODO?
    geng.mapvieworigin = -1;
}


/* Initial the window for a game. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
void TileWorldMainWnd::InitGame(gamestate &state, int nBestTime)
{
    bool const bTimedLevel = (state.game->time > 0);
    bool const bForceShowTimer = action_forceShowTimer->isChecked();
    bool bParBad = (state.game->sgflags & SGF_REPLACEABLE) != 0;

    // set properties
    m_ruleset = state.ruleset;
    m_levelNum = state.game->number;
    m_levelName = state.game->name.c_str();
    m_timedLevel = bTimedLevel;
    m_problematic = false;
    m_bestTime = nBestTime;
    m_replay = false;  // IMPORTANT for OnSpeedValueChanged
    SetSpeed(0);    // IMPORTANT

    // gui stuff
    m_gameWidget->setCursor(m_ruleset==Ruleset_MS ? Qt::CrossCursor : Qt::ArrowCursor);
    m_pLCDNumber->display(state.game->number);
    m_labelTitle->setText(m_levelPackName + " - " + m_levelName);
    m_labelPassword->setText(state.game->passwd);
    m_slideSeek->setValue(0);
    action_Pause->setText("Start");

    // easter egg
    m_oFNT = (m_levelName.toUpper() == "YOU CAN'T TEACH AN OLD FROG NEW TRICKS");

    // show/hide controls pane
    bool bHasSolution = (hassolution(state.game) && ((state.game->sgflags & SGF_REPLACEABLE) == 0));
    bool bHasDeletedSolution = (hassolution(state.game) && ((state.game->sgflags & SGF_REPLACEABLE) != 0));
    m_controlsFrame->setVisible(bHasSolution);

    // disable/enable menus
    action_Scores->setEnabled(true);
    action_TimesClipboard->setEnabled(true);
    action_Import->setEnabled(true);
    action_Levelsets->setEnabled(true);
    action_About->setEnabled(true);
    action_GoTo->setEnabled(true);
    action_Playback->setEnabled(bHasSolution);
    action_Verify->setEnabled(bHasSolution);
    action_Delete->setEnabled(hassolution(state.game));

    // pedantic mode
    action_PedanticMode->setVisible(m_ruleset == Ruleset_Lynx);
    action_PedanticMode->setEnabled(m_ruleset == Ruleset_Lynx);

    // Change delete menu option as appropriate
    if(bHasDeletedSolution) action_Delete->setText("Undelete");
    else action_Delete->setText("Delete");

    // pro- and epilogue
    CCX::Level const & currLevel(m_ccxLevelset.vecLevels[m_levelNum]);
    bool hasPrologue(!currLevel.txtPrologue.vecPages.empty());
    bool hasEpilogue(!currLevel.txtEpilogue.vecPages.empty());
    action_Prologue->setEnabled(hasPrologue);
    action_Epilogue->setEnabled(hasEpilogue && bHasSolution);

    // time
    m_progressTime->setPar(nBestTime == TIME_NIL ? -1 : nBestTime);
    m_progressTime->setParBad(bParBad);

    // set time formatting
    if (bTimedLevel) {
        if (bParBad || nBestTime == TIME_NIL) {
            m_progressTime->setFormat("%v");
            m_timeFormat  = "%v";
        } else {
            m_progressTime->setFormat("%b / %v");
            m_timeFormat  = "%v (%d)";
        }
        m_progressTime->setFullBar(false);
    } else if(bForceShowTimer) {
        if (bParBad || nBestTime == TIME_NIL) {
            m_progressTime->setFormat("[%v]");
            m_timeFormat  = "[%v]";
        } else {
            m_progressTime->setFormat("[%b] / [%v]");
            m_timeFormat  = "[%v] (%d)";
        }
        m_progressTime->setFullBar(false);
    } else {
        m_progressTime->setFormat("---");
        m_timeFormat  = "---";
        m_progressTime->setFullBar(true);
    }

    // set time limits
    int timeLimit = bTimedLevel ? state.game->time : 999;
    if (nBestTime != TIME_NIL) {
        m_slideSeek->setMaximum(timeLimit - nBestTime);
    }
    m_progressTime->setMaximum(timeLimit);
    m_progressTime->setValue(timeLimit);

    // Hide hint and set text
    SetHintVisibility(false);
    SetHintText(state.hinttext.c_str());

    // This sets m_problematic as true if there are any problems
    CheckForProblems(state);

    Narrate(&CCX::Level::txtPrologue);
}

/* Initial the window for a game. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
void TileWorldMainWnd::StartGame(gamestate &state)
{
    m_replay = (state.replay >= 0);
    m_controlsFrame->setVisible(m_replay);
    if (m_problematic) {
        SetHintVisibility(false);
        m_problematic = false;
    }

    // disable menus
    action_Scores->setEnabled(false);
    action_TimesClipboard->setEnabled(false);
    action_Import->setEnabled(false);
    action_Levelsets->setEnabled(false);
    action_Playback->setEnabled(false);
    action_Verify->setEnabled(false);
    action_Delete->setEnabled(false);
    action_About->setEnabled(false);
    action_GoTo->setEnabled(false);
    action_Prologue->setEnabled(false);
    action_Epilogue->setEnabled(false);
    action_PedanticMode->setEnabled(false);

    m_progressTime->setFormat(m_timeFormat);
}


/* Display the current game state. timeleft provide the
 * current time on the clock.
 */
void TileWorldMainWnd::DisplayGame(gamestate &state, int nTimeLeft)
{
    m_timeLeft = nTimeLeft;

    // display blank pause screen in ms mode
    if (state.statusflags & SF_SHUTTERED) {
        DisplayShutter();
    } else {
        DisplayMapView(state);
    }

    // draw objects widget
    for (int i = 0; i < 4; ++i) {
        drawfulltileid(m_invSurface, i*geng.wtile, 0,
            (state.keys[i] ? Key_Red+i : Empty));
        drawfulltileid(m_invSurface, i*geng.wtile, geng.htile,
            (state.boots[i] ? Boots_Ice+i : Empty));
    }
    m_objectsWidget->setPixmap(m_invSurface->GetPixmap());

    // chips left
    m_pLCDChipsLeft->display(state.chipsneeded);

    // time left
    m_progressTime->setValue(nTimeLeft);

    // move progress slider in replay mode
    if (m_replay && !m_slideSeek->isSliderDown()) {
        m_slideSeek->blockSignals(true);
        m_slideSeek->setValue(state.currenttime / TICKS_PER_SECOND);
        m_slideSeek->blockSignals(false);
    }

    // set the hint when on a relevant tile
    if (!m_problematic) {
        // Call setText / clear only when really required
        // See comments about QLabel in TWDisplayWidget.h
        if ((state.statusflags & SF_SHOWHINT) != 0) {
            SetHintVisibility(true);
        } else {
            SetHintVisibility(false);
        }
    }
}

void TileWorldMainWnd::CheckForProblems(const gamestate &state)
{
    QString s;

    if (state.statusflags & SF_INVALID) {
        s = "This level cannot be played.";
    } else if (state.game->unsolvable) {
        s = "This level is reported to be unsolvable";
        if (!state.game->unsolvablereason.empty())
            s += ": " + QString(state.game->unsolvablereason.c_str());
        s += ".";
    } else {
        CCX::RulesetCompatibility ruleCompat = m_ccxLevelset.vecLevels[m_levelNum].ruleCompat;
        CCX::Compatibility compat = CCX::COMPAT_UNKNOWN;
        if (m_ruleset == Ruleset_Lynx) {
            if (pedanticmode)
                compat = ruleCompat.ePedantic;
            else
                compat = ruleCompat.eLynx;
        } else if (m_ruleset == Ruleset_MS) {
            compat = ruleCompat.eMS;
        }

        if (compat == CCX::COMPAT_NO){
            s = "This level is flagged as being incompatible with the current ruleset.";
        }
    }

    m_problematic = !s.isEmpty();
    if (m_problematic) {
        SetHintText(s);
        SetHintVisibility(true);
    }
}

void TileWorldMainWnd::DisplayMapView(gamestate &state)
{
    short xviewpos = state.xviewpos;
    short yviewpos = state.yviewpos;
    bool bFrogShow = (m_oFNT  &&  m_replay  &&
                    xviewpos/8 == 14  &&  yviewpos/8 == 9);
    if (bFrogShow) {
        int x = xviewpos, y = yviewpos;
        if (m_ruleset == Ruleset_MS) {
            for (int pos = 0; pos < CXGRID*CYGRID; ++pos) {
                int id = state.map[pos].top.id;
                if ( ! (id >= Teeth && id < Teeth+4) )
                    continue;
                x = (pos % CXGRID) * 8;
                y = (pos / CXGRID) * 8;
                break;
            }
        } else {
            for (const creature* p = state.creatures; p->id != 0; ++p) {
                if ( ! (p->id >= Teeth && p->id < Teeth+4) )
                    continue;
                x = (p->pos % CXGRID) * 8;
                y = (p->pos / CXGRID) * 8;
                if (p->moving > 0) {
                    switch (p->dir) {
                        case NORTH: y += p->moving; break;
                        case WEST:  x += p->moving; break;
                        case SOUTH: y -= p->moving; break;
                        case EAST:  x -= p->moving; break;
                    }
                }
                break;
            }
        }
        state.xviewpos = x;
        state.yviewpos = y;
    }

    displaymapview(state, m_disploc);
    m_gameWidget->setPixmap(m_surface->GetPixmap());

    if (bFrogShow) {
        state.xviewpos = xviewpos;
        state.yviewpos = yviewpos;
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

    m_gameWidget->setPixmap(pixmap);
}


void TileWorldMainWnd::OnSpeedValueChanged(int nValue)
{
    // IMPORTANT!
    if (!m_replay) return;
    // Even though the replay controls are hidden when play begins,
    //  the slider could be manipulated before making the first move

    SetSpeed(nValue);
}

void TileWorldMainWnd::SetSpeed(int nValue)
{
    int nMS = (m_ruleset == Ruleset_MS) ? 1100 : 1000;
    if (nValue >= 0)
        settimersecond(nMS >> nValue);
    else
        settimersecond(nMS << (-nValue/2));
}

void TileWorldMainWnd::OnSpeedSliderReleased()
{
    m_slideSpeed->setValue(0);
}


/* Get number of seconds to skip at start of playback.
 */
int TileWorldMainWnd::GetReplaySecondsToSkip() const
{
    return m_slideSeek->value();
}


void TileWorldMainWnd::OnSeekPosChanged(int nValue)
{
    PulseKey(CmdSeek);
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

    if (nCompleted == -2)   // abandoned
        return CmdNone;

    QMessageBox msgBox(this);

    if (nCompleted > 0)  { // Success
        QString sText;
        QTextStream strm(&sText);
        strm.setLocale(m_locale);
        strm << "<big><b>" << m_levelName << "</b></big><br>";

        QString sAuthor = m_ccxLevelset.vecLevels[m_levelNum].sAuthor;
        if (!sAuthor.isEmpty())
            strm << "by " << sAuthor;

        strm << "<hr><br><big><b>";
        if (m_replay) {
            strm << "Alright!";
        } else {
            strm << getmessage(MessageWin, "You won!");
        }
        strm << "</b></big><br>";

        if (!m_replay) {
            if (m_timedLevel && m_bestTime != TIME_NIL) {
                int diff = m_timeLeft - m_bestTime;

                if (diff == 0)
                    strm << "You scored " << m_bestTime << " yet again.";
                else if (diff == 1)
                    strm << "You made it 1 second faster this time!";
                else if (diff > 0)
                    strm << "You made it " << diff << " seconds faster this time!";
                else
                    strm << "But not as quick as your previous score of " << m_bestTime << "...";
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

        msgBox.setWindowTitle(m_replay ? "Replay Completed" : "Level Completed");

        m_textToCopy = timestring(m_levelNum, m_levelName, m_timeLeft, m_timedLevel, false);

        msgBox.addButton("&Onward!", QMessageBox::AcceptRole);
        QPushButton* pBtnRestart = msgBox.addButton("&Restart", QMessageBox::AcceptRole);
        QPushButton* pBtnCopyScore = msgBox.addButton("&Copy Score", QMessageBox::ActionRole);
        connect( pBtnCopyScore, SIGNAL(clicked()), this, SLOT(OnCopyText()) );

        msgBox.exec();
        ReleaseAllKeys();
        if (msgBox.clickedButton() == pBtnRestart)
            return CmdSameLevel;

        Narrate(&CCX::Level::txtEpilogue);
    } else {    // Failure
        bool bTimeout = (m_timedLevel  &&  m_timeLeft <= 0);
        if (m_replay) {
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
                QStyle* pStyle = g_app->style();
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
int TileWorldMainWnd::DisplayList(TWTableSpec &table, int &pnIndex,
        bool showRulesetOptions, int *ruleset /* = nullptr */)
{
    int nCmd = 0;
    QAction *actions[] = { action_Scores, action_TimesClipboard,
                           action_Import, action_Levelsets};
    bool action_status[4];
    for(int i = 0; i < 4; i++) {
        action_status[i] = actions[i]->isEnabled();
        actions[i]->setEnabled(false);
    }

    QMenu *menus[] = { menu_Level, menu_Solution, menu_Options, menu_Zoom, menu_Solution};
    bool menu_status[5];
    for(int i = 0; i < 5; i++) {
        menu_status[i] = menus[i]->isEnabled();
        menus[i]->setEnabled(false);
    }

    // dummy scope to force table spec destructors before ExitTWorld
    {
        table.fixRows();
        QSortFilterProxyModel proxyModel;
        m_sortFilterProxyModel = &proxyModel;
        proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel.setFilterKeyColumn(-1);
        proxyModel.setSourceModel(&table);
        m_tableList->setModel(&proxyModel);

        m_tableList->horizontalHeader()->setStretchLastSection(table.cols() == 1);

        QModelIndex index = proxyModel.mapFromSource(table.index(pnIndex, 0));
        m_tableList->setCurrentIndex(index);
        m_tableList->resizeColumnsToContents();
        m_tableList->resizeRowsToContents();
        m_textFind->clear();
        SetCurrentPage(PAGE_TABLE);
        m_tableList->setFocus();

        m_radioMS->setVisible(showRulesetOptions);
        m_radioLynx->setVisible(showRulesetOptions);
        m_goButton->setVisible(showRulesetOptions);

        if(ruleset != nullptr) {
            if(*ruleset == Ruleset_MS)
                m_radioMS->setChecked(true);
            else
                m_radioLynx->setChecked(true);
        }

        nCmd = g_app->exec();

        pnIndex = proxyModel.mapToSource(m_tableList->currentIndex()).row();

        SetCurrentPage(PAGE_GAME);
        m_tableList->setModel(0);
        m_sortFilterProxyModel = 0;

        if(ruleset != nullptr) {
            *ruleset = m_radioMS->isChecked() ? Ruleset_MS : Ruleset_Lynx;
        }
    }

    if (m_windowClosed) g_app->ExitTWorld();

    // restore menus and menu items to previous value
    for(int i = 0; i < 4; i++) {
        actions[i]->setEnabled(action_status[i]);
    }
    for(int i = 0; i < 5; i++) {
        menus[i]->setEnabled(menu_status[i]);
    }

    return nCmd;
}

void TileWorldMainWnd::OnListItemActivated()
{
    g_app->exit(CmdProceed);
}

void TileWorldMainWnd::OnFindTextChanged(const QString& sText)
{
    if (!m_sortFilterProxyModel) return;

    QString sWildcard;
    if (sText.isEmpty())
        sWildcard = "*";
    else
        sWildcard = '*' + sText + '*';
    m_sortFilterProxyModel->setFilterWildcard(sWildcard);
}

void TileWorldMainWnd::OnFindReturnPressed()
{
    if (!m_sortFilterProxyModel) return;

    int n = m_sortFilterProxyModel->rowCount();
    if (n == 0) {
        TileWorldApp::Bell();
        return;
    }

    m_tableList->setFocus();

    if (!m_tableList->currentIndex().isValid())
        m_tableList->selectRow(0);

    if (n == 1)
        g_app->exit(CmdProceed);
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


void TileWorldMainWnd::DisplayPasswordPrompt(char *passwd)
{
    QString password = QInputDialog::getText(this, TileWorldApp::applicationName(), "Enter Password");
    password.truncate(4);
    password = password.toUpper();
    strncpy(passwd, password.toUtf8().constData(), 4);
    passwd[4] = '\0';
}

/*
 * The subtitle stack
 */
void TileWorldMainWnd::PushSubtitle(const QString &subtitle)
{
    m_subtitlestack.append(subtitle);
    SetSubtitle(subtitle);
}

void TileWorldMainWnd::PopSubtitle()
{
    if(!m_subtitlestack.isEmpty()) {
        m_subtitlestack.removeLast();
    }

    if(!m_subtitlestack.isEmpty()) {
        SetSubtitle(m_subtitlestack.last());
    } else {
        SetSubtitle("");
    }
}

void TileWorldMainWnd::ChangeSubtitle(const QString &subtitle)
{
    if(!m_subtitlestack.isEmpty()) {
        m_subtitlestack.last() = subtitle;
    }
    SetSubtitle(subtitle);
}


/* Set the program's subtitle. A nullptr subtitle is equivalent to the
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

/* Read any additional data for the series.
 */
void TileWorldMainWnd::ReadExtensions(gameseries &pSeries)
{
    QDir dataDir;
    dataDir.setPath(getdir(pSeries.mapfiledir));

    QString sSetName = QFileInfo(pSeries.mapfilename.c_str()).completeBaseName();
    m_levelPackName = sSetName; // save for use on display

    QString sFilePath = dataDir.filePath(sSetName + ".ccx");

    m_ccxLevelset.Clear();
    if (!m_ccxLevelset.ReadFile(sFilePath, pSeries.count))
        warn("%s: failed to read file", sFilePath.toUtf8().constData());

    for (int i = 1; i <= pSeries.count; ++i) {
        CCX::Level& rCCXLevel = m_ccxLevelset.vecLevels[i];
        rCCXLevel.txtPrologue.bSeen = false;    // @#$ (pSeries.games[i-1].sgflags & SGF_HASPASSWD) != 0;
        rCCXLevel.txtEpilogue.bSeen = false;
    }
}


void TileWorldMainWnd::Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce)
{
    CCX::Text& rText = m_ccxLevelset.vecLevels[m_levelNum].*pmTxt;
    if ((rText.bSeen || !action_displayCCX->isChecked()) && !bForce)
        return;
    rText.bSeen = true;

    if (rText.vecPages.empty())
        return;
    int n = rText.vecPages.size();

    QString sWindowTitle = this->windowTitle();
    SetSubtitle("");    // TODO: set name
    SetCurrentPage(PAGE_TEXT);
    m_buttonTextNext->setFocus();

    int d = +1;
    for (int nPage = 0; nPage < n; nPage += d) {
        m_buttonTextPrev->setVisible(nPage > 0);

        CCX::Page& rPage = rText.vecPages[nPage];

        QTextDocument* pDoc = m_textBrowser->document();
        if (pDoc != 0) {
            if (!m_ccxLevelset.sStyleSheet.isEmpty())
                pDoc->setDefaultStyleSheet(m_ccxLevelset.sStyleSheet);
            pDoc->setDocumentMargin(16);
        }

        QString sText = rPage.sText;
        if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN) {
            m_textBrowser->setPlainText(sText);
        } else {
            m_textBrowser->setHtml(sText);
        }

        d = g_app->exec();
        if (m_windowClosed) g_app->ExitTWorld();
        if (d == 0) // Return
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
}

void TileWorldMainWnd::OnTextNext()
{
    g_app->exit(+1);
}

void TileWorldMainWnd::OnTextPrev()
{
    g_app->exit(-1);
}

void TileWorldMainWnd::OnTextReturn()
{
    g_app->exit(0);
}


void TileWorldMainWnd::OnCopyText()
{
    QClipboard* pClipboard = QApplication::clipboard();
    if (pClipboard == 0) return;
    pClipboard->setText(m_textToCopy);
}

void TileWorldMainWnd::OnMenuActionTriggered(QAction* pAction)
{
    if (pAction == action_Exit) {
        g_app->ExitTWorld();
        return;
    }

    if (pAction == action_Prologue) {
        Narrate(&CCX::Level::txtPrologue, true);
        return;
    }

    if (pAction == action_Epilogue) {
        Narrate(&CCX::Level::txtEpilogue, true);
        return;
    }

    if (pAction == action_Import) {
        OnImportButton();
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

        if (m_ruleset == Ruleset_Lynx) {
            stepDialog.setComboBoxItems(m_stepDialogOptions);
        } else {
            stepDialog.setComboBoxItems({m_stepDialogOptions[0], m_stepDialogOptions[4]});
        }

        int step = getstepping();
        stepDialog.setTextValue(m_stepDialogOptions[step]);

        stepDialog.exec();

        int stepIndex = m_stepDialogOptions.indexOf(stepDialog.textValue());
        setstepping(stepIndex);
        return;
    }

    if (pAction == action_PedanticMode) {
        setpedanticmode(pAction->isChecked());
        return;
    }

    int cmd = GetCmdForAction(pAction);
    if (cmd == CmdNone) return;
    PulseKey(cmd);
}

int TileWorldMainWnd::GetCmdForAction(QAction* pAction) const
{
    if (pAction == action_Scores) return CmdSeeScores;
    if (pAction == action_TimesClipboard) return CmdTimesClipboard;
    if (pAction == action_Levelsets) return CmdQuitLevel;
    if (pAction == action_Exit) return CmdQuit;

    if (pAction == action_Pause) return CmdPauseGame;
    if (pAction == action_Restart) return CmdSameLevel;
    if (pAction == action_Next) return CmdNextLevel;
    if (pAction == action_Previous) return CmdPrevLevel;
    if (pAction == action_GoTo) return CmdGotoLevel;

    if (pAction == action_Playback) return CmdPlayback;
    if (pAction == action_Verify) return CmdCheckSolution;
    if (pAction == action_Delete) return CmdDelSolution;

    return CmdNone;
}

void TileWorldMainWnd::ResizeHintFont()
{
    SetHintText(m_labelHint->text());
}

void TileWorldMainWnd::SetHintText(const QString &hint)
{
    // Calculate available dimensions for hint
    int availableHeight = m_labelTitle->geometry().bottom() - m_objectsContainer->geometry().y();
    int availableWidth = m_infoFrame->width();
    int margins = (m_labelHint->margin() + m_messagesFrame->frameWidth()) * 2;
    availableHeight -= margins;
    availableWidth -= margins;

    // decrease font size
    QFont thisFont = m_labelHint->font();
    for(int fs=25; fs > 12; fs--) {
        thisFont.setPixelSize(fs);
        QFontMetrics fm(thisFont);
        QRect r = fm.boundingRect(0, 0, availableWidth, 1000, Qt::TextWordWrap, hint);

        if(r.height() <= availableHeight) break;
    }

    m_labelHint->setFont(thisFont);
    m_labelHint->setText(hint);
}

void TileWorldMainWnd::SetHintVisibility(bool newmode)
{
    bool changed = (newmode != m_hintVisible);
    m_hintVisible = newmode;

    if(!changed) {
        return;
    } else if(m_hintVisible) {
        m_infoPane->setCurrentIndex(1);
    } else {
        m_infoPane->setCurrentIndex(0);
    }
}

void TileWorldMainWnd::SetScale(int s, bool checkPrevScale)
{
    double newScale = (double)s / 100;
    if(checkPrevScale && newScale == m_scale) return;

    // set the property
    m_scale = sqrt(newScale);

    if(m_surface == 0 || m_invSurface == 0 || geng.wtile < 1) {
        warn("Attempt to set pixmap and m_scale without setting pixmap first");
        return;
    }

    // Hide the hint to avoid knock-on layout issues
    if(m_hintVisible == true) {
        SetHintVisibility(false);
    }

    // align widget size to specified m_scale
    m_gameWidget->setFixedSize(m_scale * DEFAULTTILE * NXTILES, m_scale * DEFAULTTILE * NYTILES);
    m_objectsWidget->setFixedSize(m_scale * DEFAULTTILE * 4, m_scale * DEFAULTTILE * 2);

    // this sets the game and objects box
    m_gameWidget->setPixmap(m_surface->GetPixmap());
    m_objectsWidget->setPixmap(m_invSurface->GetPixmap());

    // this aligns the width of the objects box with the other elements
    // in the right column
    m_messagesFrame->setFixedWidth((4 * DEFAULTTILE * m_scale) + 10);
    m_infoFrame->setFixedWidth((4 * DEFAULTTILE * m_scale) + 10);

    // we need to determine hint font size again
    ResizeHintFont();
}

void TileWorldMainWnd::SetPlayPauseButton(bool paused)
{
    if(paused) {
        m_buttonPlay->setIcon(m_playIcon);
        action_Pause->setText("Resume");
    } else {
        m_buttonPlay->setIcon(m_pauseIcon);
        action_Pause->setText("Pause");
    }
}

void TileWorldMainWnd::ChangeVolume(int volume)
{
    if(m_volTimer->isActive()) m_volTimer->stop();

    m_progressVolFrame->setVisible(true);
    m_progressVolume->setValue(changevolume(volume));

    connect(m_volTimer, SIGNAL(timeout()), this, SLOT(HideVolumeWidget()));

    m_volTimer->setSingleShot(true);
    m_volTimer->start(2000);
}

void TileWorldMainWnd::HideVolumeWidget()
{
    m_progressVolFrame->setVisible(false);
}


/* This callback is called whenever the state of any keyboard key
 * changes. It records this change in the m_keystates array. The key can
 * be recorded as being struck, pressed, repeating, held down, or down
 * but ignored, as appropriate to when they were first pressed and the
 * current behavior settings. Shift-type keys are always either on or
 * off.
 */
void TileWorldMainWnd::KeyEventCallback(int scancode, bool down)
{
    if (down) {
        m_keystates[scancode] = m_keystates[scancode] == KS_OFF ? KS_PRESSED : KS_REPEATING;
    } else {
        m_keystates[scancode] = m_keystates[scancode] == KS_PRESSED ? KS_STRUCK : KS_OFF;
    }
}

/* Initialize (or re-initialize) all key states.
 */
void TileWorldMainWnd::RestartKeystates(void)
{
    memset(m_keystates, KS_OFF, sizeof m_keystates);
    for (int n = 0; n < TWK_LAST; ++n)
    if (m_keyState[n])
        KeyEventCallback(n, true);
}

/* Update the key states. This is done at the start of each polling
 * cycle. The state changes that occur depend on the current behavior
 * settings.
 */
void TileWorldMainWnd::ResetKeyStates(void)
{
    for (int n = 0 ; n < TWK_LAST ; ++n) {
        int x = (int)m_keystates[n];

        if(x == KS_STRUCK) {
            m_keystates[n] = KS_OFF;
        } else if(x == KS_DOWNBUTOFF2 || x == KS_DOWNBUTOFF3 || x == KS_REPEATING) {
            m_keystates[n] = KS_DOWN;
        } else if(x == KS_PRESSED) {
            m_keystates[n] = m_joystickstyle ? KS_DOWN : KS_DOWNBUTOFF1;
        } else if(x == KS_DOWNBUTOFF1) {
            m_keystates[n] = m_joystickstyle ? KS_DOWN : KS_DOWNBUTOFF2;
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

    double t = DEFAULTTILE * m_scale;
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
    switch (m_mouseinfo.state) {
        case KS_PRESSED:
            m_mouseinfo.state = KS_OFF;
            if (m_mouseinfo.button == Qt::LeftButton) {
                int n = WindowMapPos(m_mouseinfo.x, m_mouseinfo.y);
                if (n >= 0) {
                    m_mouseinfo.state = KS_DOWNBUTOFF1;
                    return CmdAbsMouseMoveFirst + n;
                }
            }
            break;
        case KS_DOWNBUTOFF1:
            m_mouseinfo.state = KS_DOWNBUTOFF2;
            return CmdPreserve;
        case KS_DOWNBUTOFF2:
            m_mouseinfo.state = KS_DOWNBUTOFF3;
            return CmdPreserve;
        case KS_DOWNBUTOFF3:
            m_mouseinfo.state = KS_OFF;
            return CmdPreserve;
    }
    return 0;
}

/* Poll the keyboard and return the command associated with the
 * selected key, if any. If no key is selected and wait is TRUE, block
 * until a key with an associated command is selected. In keyboard behavior
 * mode, the function can return CmdPreserve, indicating that if the key
 * command from the previous poll has not been processed, it should still
 * be considered active. If two m_mergeable keys are selected, the return
 * value will be the bitwise-or of their command values.
 */
int TileWorldMainWnd::Input(bool wait)
{
    keycmdmap const    *kc;
    bool        lingerflag = false;
    int         cmd1, cmd, n;

    for (;;) {
        ResetKeyStates();
        eventupdate(wait);

        if (m_nextcommand != CmdNone) {
            int nextcommand = m_nextcommand;
            m_nextcommand = CmdNone;
            return nextcommand;
        }

        cmd1 = cmd = 0;
        for (kc = keycmds ; kc->scancode ; ++kc) {
            n = m_keystates[kc->scancode];
            if (!n) continue;
            if (n == KS_PRESSED || (kc->hold && n == KS_DOWN)) {
                if (!cmd1) {
                    cmd1 = kc->cmd;
                    if (!m_joystickstyle || cmd1 > CmdKeyMoveLast || !m_mergeable[cmd1])
                        return cmd1;
                } else {
                    if (cmd1 <= CmdKeyMoveLast && (m_mergeable[cmd1] & kc->cmd) == kc->cmd)
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
    m_joystickstyle = enable;
    RestartKeystates();
    return true;
}
