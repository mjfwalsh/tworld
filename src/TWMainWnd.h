/* Copyright (C) 2001-2019 by Madhav Shanbhag, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWMAINWND_H
#define TWMAINWND_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QLocale>

#include "../obj/ui_TWMainWnd.h"
#include "CCMetaData.h"
#include "defs.h"

class TWTableSpec;
struct gamestate;
class QWindow;

class QSortFilterProxyModel;

class TileWorldMainWnd : public QMainWindow, protected Ui::TWMainWnd
{
    Q_OBJECT

public:
    enum Page
    {
        PAGE_GAME,
        PAGE_TABLE,
        PAGE_TEXT
    };

    /* Structure describing mouse activity.
     */
    typedef struct mouseaction {
        int     state;      /* state of mouse action (KS_*) */
        int     x, y;       /* position of the mouse */
        int     button;     /* which button generated the event */
    } mouseaction;

    /* The possible states of keys.
     */
    enum { KS_OFF = 0,      /* key is not currently pressed */
           KS_ON = 1,       /* key is down (shift-type keys only) */
           KS_DOWN,         /* key is being held down */
           KS_STRUCK,       /* key was pressed and released in one tick */
           KS_PRESSED,      /* key was pressed in this tick */
           KS_DOWNBUTOFF1,      /* key has been down since the previous tick */
           KS_DOWNBUTOFF2,      /* key has been down since two ticks ago */
           KS_DOWNBUTOFF3,      /* key has been down since three ticks ago */
           KS_REPEATING,        /* key is down and is now repeating */
           KS_count
    };

    /* The possible keys / commands
     */
    enum
    {
        TWK_LEFT = 1, // TWMainWnd.cpp
        TWK_UP,
        TWK_RIGHT,
        TWK_DOWN,
    #ifndef NDEBUG
        TWK_LEFT_CHEAT,
        TWK_UP_CHEAT,
        TWK_RIGHT_CHEAT,
        TWK_DOWN_CHEAT,
    #endif
        TWK_RETURN,// TWMainWnd.cpp
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

        TWK_LAST
    };

    /* Structure describing a mapping of a key event to a game command.
     */
    typedef struct keycmdmap {
        int     scancode;   /* the key's scan code */
        int     cmd;        /* the command */
        bool    hold;       /* TRUE for repeating joystick-mode keys */
    } keycmdmap;

    /* The complete list of key commands recognized by the game while
     * playing. hold is TRUE for keys that are to be forced to repeat.
     * shift, ctl and alt are positive if the key must be down, zero if
     * the key must be up, or negative if it doesn't matter.
     */
    static keycmdmap constexpr keycmds[] = {
        {   TWK_UP,                 CmdNorth,               true    },
        {   TWK_LEFT,               CmdWest,                true    },
        {   TWK_DOWN,               CmdSouth,               true    },
        {   TWK_RIGHT,              CmdEast,                true    },
        {   TWK_RETURN,             CmdProceed,             false   },

#ifndef NDEBUG
        {   TWK_DEBUG1,             CmdDebugCmd1,           false   },
        {   TWK_DEBUG2,             CmdDebugCmd2,           false   },

        {   TWK_CHIP,               CmdCheatICChip,         false   },
        {   TWK_RED,                CmdCheatKeyRed,         false   },
        {   TWK_BLUE,               CmdCheatKeyBlue,        false   },
        {   TWK_YELLOW,             CmdCheatKeyYellow,      false   },
        {   TWK_GREEN,              CmdCheatKeyGreen,       false   },

        {   TWK_ICE,                CmdCheatBootsIce,       false   },
        {   TWK_SLIDE,              CmdCheatBootsSlide,     false   },
        {   TWK_FIRE,               CmdCheatBootsFire,      false   },
        {   TWK_WATER,              CmdCheatBootsWater,     false   },

        {   TWK_UP_CHEAT,           CmdCheatNorth,          true    },
        {   TWK_LEFT_CHEAT,         CmdCheatWest,           true    },
        {   TWK_DOWN_CHEAT,         CmdCheatSouth,          true    },
        {   TWK_RIGHT_CHEAT,        CmdCheatEast,           true    },
#endif
        {   0,  0,  0   }
    };

    TileWorldMainWnd(QWidget* pParent = 0);
    ~TileWorldMainWnd();

    virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
    virtual void closeEvent(QCloseEvent* pCloseEvent) override;

    void SetKeyboardRepeat(bool bEnable);
    int GetReplaySecondsToSkip() const;

    void CreateGameDisplay();
    void ClearDisplay();
    void InitGame(gamestate &state, int nBestTime, const char *levelPackName, const QString &author, const QString &problem);
    void StartGame(gamestate &state);
    void DisplayGame(gamestate &state, int nTimeLeft);
    int DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted);
    void DisplayList(TWTableSpec &pTableSpec, int pnIndex, int ruleset);
    void HideList();
    int GetSelectedRuleSet();
    int GetSelectedRow();

    bool DisplayYesNoPrompt(const char* prompt);
    void DisplayPasswordPrompt(char *passwd);

    void ReadExtensions(gameseries &pSeries);

    void ShowAbout();
    void SetPlayPauseButton(bool p);
    int Input(bool wait);
    bool SetKeyboardArrowsRepeat(bool enable);
    void ReleaseAllKeys();

    void ChangeSubtitle(const QString &subtitle);
    void PopSubtitle();
    void PushSubtitle(const QString &subtitle);
    bool GetAutoShowNarration();

    class Narration
    {
    public:
        Narration(TileWorldMainWnd *p, CCX::Text &t, QString &styleSheet)
        :
          parent(p),
          text(t)
        {
              parent->PushSubtitle("");
              parent->SetCurrentPage(PAGE_TEXT);
              parent->m_buttonTextNext->setFocus();

              QTextDocument* pDoc = parent->m_textBrowser->document();
              if (!styleSheet.isEmpty())
                  pDoc->setDefaultStyleSheet(styleSheet);
              pDoc->setDocumentMargin(16);
              ChangePage(0);
        }

        ~Narration()
        {
              parent->PopSubtitle();
              parent->SetCurrentPage(PAGE_GAME);
        }

        bool ChangePage(int delta)
        {
            page += delta;
            if (page < 0 || page >= text.vecPages.size())
                return false;

            parent->m_buttonTextPrev->setVisible(page > 0);

            CCX::Page &rPage = text.vecPages[page];

            if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN)
                parent->m_textBrowser->setPlainText(rPage.sText);
            else
                parent->m_textBrowser->setHtml(rPage.sText);

            return true;
        }

    private:
        TileWorldMainWnd *parent;
        CCX::Text &text;
        int page = 0;
    };

public slots:
    void HideVolumeWidget();
    void FocusChanged(QWindow *w);

private slots:
    void OnListItemActivated();
    void OnFindTextChanged(const QString& sText);
    void OnFindReturnPressed();
    void OnPlayback();
    void OnSpeedValueChanged(int nValue);
    void OnSpeedSliderReleased();
    void OnSeekPosChanged(int nValue);
    void OnTextNext();
    void OnTextPrev();
    void OnTextReturn();
    void OnCopyText();
    void OnMenuActionTriggered(QAction* pAction);
    void OnBackButton();
    void OnImportButton();
    void SetSubtitle(QString subtitle);

private:
    bool HandleKeyEvent(QObject* pObject, QKeyEvent* pEvent);
    void HandleMouseEvent(QMouseEvent* pEvent);
    void SetCurrentPage(Page ePage);
    void DisplayMapView(gamestate &state);
    void DisplayShutter();
    void SetSpeed(int nValue);
    void PulseKey(int nTWKey);
    int GetCmdForAction(QAction* pAction) const;
    void ChangeVolume(int volume);

    int RetrieveMouseCommand(void);
    int WindowMapPos(int x, int y);
    void ResetKeyStates(void);
    void RestartKeystates(void);
    void KeyEventCallback(int scancode, bool down);

    // The complete array of key states.
    char m_keystates[TWK_LAST];

    int m_nextcommand = CmdNone;

    // The last mouse action.
    mouseaction m_mouseinfo;

    // TRUE if direction keys are to be treated as always repeating.
    bool m_joystickstyle = false;

    // A map of keys that can be held down simultaneously to produce
    // multiple commands.
    int m_mergeable[CmdKeyMoveLast + 1];

    void ResizeHintFont();
    void SetHintText(const QString &hint);
    void SetHintVisibility(bool newmode);
    void SetScale(int s, bool checkPrevScale = true);

    bool m_windowClosed;

    Qt_Surface* m_surface;
    Qt_Surface* m_invSurface;
    TW_Rect m_disploc;

    double m_scale = 1;

    bool m_keyState[TWK_LAST];

    bool m_kbdRepeatEnabled;

    int m_ruleset;
    int m_levelNum;
    QString m_levelName;
    QString m_levelPackName;
    QString m_timeFormat;
    bool m_problematic;
    bool m_oFNT;
    int m_bestTime;
    bool m_hintVisible;
    int m_timeLeft;
    bool m_timedLevel;
    bool m_replay;
    QString m_author;

    QSortFilterProxyModel* m_sortFilterProxyModel;
    QLocale m_locale;

    QString m_textToCopy;

    QIcon m_playIcon;
    QIcon m_pauseIcon;

    QTimer *m_volTimer;

    QStringList m_stepDialogOptions = {
    "Even Step",
    "Even Step +1",
    "Even Step +2",
    "Even Step +3",
    "Odd Step",
    "Odd Step +1",
    "Odd Step +2",
    "Odd Step +3"};

    /* The top of the stack of subtitles.
     */
    QStringList m_subtitlestack;
};

extern TileWorldMainWnd* g_mainWindow;

#endif
