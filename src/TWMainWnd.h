/* Copyright (C) 2001-2019 by Madhav Shanbhag, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWMAINWND_H
#define TWMAINWND_H

#include <QMainWindow>
#include <QLocale>

#include "../obj/ui_TWMainWnd.h"
#include "CCMetaData.h"
#include "defs.h"

class TWTableSpec;
struct gamestate;

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
		int		state;		/* state of mouse action (KS_*) */
		int		x, y;		/* position of the mouse */
		int		button;		/* which button generated the event */
	} mouseaction;

	/* The possible states of keys.
	 */
	enum { KS_OFF = 0,		/* key is not currently pressed */
		   KS_ON = 1,		/* key is down (shift-type keys only) */
		   KS_DOWN,			/* key is being held down */
		   KS_STRUCK,		/* key was pressed and released in one tick */
		   KS_PRESSED,		/* key was pressed in this tick */
		   KS_DOWNBUTOFF1,		/* key has been down since the previous tick */
		   KS_DOWNBUTOFF2,		/* key has been down since two ticks ago */
		   KS_DOWNBUTOFF3,		/* key has been down since three ticks ago */
		   KS_REPEATING,		/* key is down and is now repeating */
		   KS_count
	};

	/* Structure describing a mapping of a key event to a game command.
	 */
	typedef	struct keycmdmap {
		int		scancode;	/* the key's scan code */
		int		cmd;		/* the command */
		bool	hold;		/* TRUE for repeating joystick-mode keys */
	} keycmdmap;

	/* The complete list of key commands recognized by the game while
	 * playing. hold is TRUE for keys that are to be forced to repeat.
	 * shift, ctl and alt are positive if the key must be down, zero if
	 * the key must be up, or negative if it doesn't matter.
	 */
	static keycmdmap constexpr keycmds[] = {
		{	TWK_UP,					CmdNorth,				true	},
		{	TWK_LEFT,				CmdWest,				true	},
		{	TWK_DOWN,				CmdSouth,				true	},
		{	TWK_RIGHT,				CmdEast,				true	},
		{	TWK_RETURN,				CmdProceed,				false	},

		{	TWC_SEESCORES,			CmdSeeScores,			false	},
		{	TWC_SEESOLUTIONFILES,	CmdSeeSolutionFiles,	false	},
		{	TWC_TIMESCLIPBOARD,		CmdTimesClipboard,		false	},
		{	TWC_QUITLEVEL,			CmdQuitLevel,			false	},
		{	TWC_QUIT,				CmdQuit,				false	},

		{	TWC_PAUSEGAME,			CmdPauseGame,			false	},
		{	TWC_SAMELEVEL,			CmdSameLevel,			false	},
		{	TWC_NEXTLEVEL,			CmdNextLevel,			false	},
		{	TWC_PREVLEVEL,			CmdPrevLevel,			false	},
		{	TWC_GOTOLEVEL,			CmdGotoLevel,			false	},

		{	TWC_PLAYBACK,			CmdPlayback,			false	},
		{	TWC_CHECKSOLUTION,		CmdCheckSolution,		false	},
		{	TWC_DELSOLUTION,		CmdDelSolution,			false	},
		{	TWC_SEEK,				CmdSeek,				false	},

#ifndef NDEBUG
		{	TWK_DEBUG1,				CmdDebugCmd1,			false	},
		{	TWK_DEBUG2,				CmdDebugCmd2,			false	},

		{	TWK_CHIP,				CmdCheatICChip,			false	},
		{	TWK_RED,				CmdCheatKeyRed,			false	},
		{	TWK_BLUE,				CmdCheatKeyBlue,		false	},
		{	TWK_YELLOW,				CmdCheatKeyYellow,		false	},
		{	TWK_GREEN,				CmdCheatKeyGreen,		false	},

		{	TWK_ICE,				CmdCheatBootsIce,		false	},
		{	TWK_SLIDE,				CmdCheatBootsSlide,		false	},
		{	TWK_FIRE,				CmdCheatBootsFire,		false	},
		{	TWK_WATER,				CmdCheatBootsWater,		false	},

		{	TWK_UP_CHEAT,			CmdCheatNorth,			true	},
		{	TWK_LEFT_CHEAT,			CmdCheatWest,			true	},
		{	TWK_DOWN_CHEAT,			CmdCheatSouth,			true	},
		{	TWK_RIGHT_CHEAT,		CmdCheatEast,			true	},
#endif
		{	0,	0,	0	}
	};

	TileWorldMainWnd(QWidget* pParent = 0, Qt::WindowFlags flags = 0);
	~TileWorldMainWnd();

	virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	virtual void closeEvent(QCloseEvent* pCloseEvent) override;

	void SetKeyboardRepeat(bool bEnable);
	int GetReplaySecondsToSkip() const;

	void CreateGameDisplay();
	void ClearDisplay();
	void DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime);
	int DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted);
	int DisplayList(TWTableSpec* pTableSpec, int* pnIndex, bool showRulesetOptions);
	bool DisplayYesNoPrompt(const char* prompt);
	const char* DisplayPasswordPrompt();
	int GetSelectedRuleset();

	void ReadExtensions(gameseries* pSeries);
	void Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce = false);

	void ShowAbout();
	void SetPlayPauseButton(bool p);
	int Input(bool wait);
	bool SetKeyboardArrowsRepeat(bool enable);

	void ChangeSubtitle(QString subtitle);
	void PopSubtitle();
	void PushSubtitle(QString subtitle);

public slots:
	void HideVolumeWidget();

private slots:
	void OnListItemActivated(const QModelIndex& index);
	void OnFindTextChanged(const QString& sText);
	void OnFindReturnPressed();
	void OnRulesetSwitched(QString checked);
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
	void SetSubtitle(QString subtitle);

private:
	bool HandleKeyEvent(QObject* pObject, QEvent* pEvent);
	bool HandleMouseEvent(QObject* pObject, QEvent* pEvent);
	void SetCurrentPage(Page ePage);
	void CheckForProblems(const gamestate* pState);
	void DisplayMapView(const gamestate* pState);
	void DisplayShutter();
	void SetSpeed(int nValue);
	void ReleaseAllKeys();
	void PulseKey(int nTWKey);
	int GetTWKeyForAction(QAction* pAction) const;
	void ChangeVolume(int volume);

	int RetrieveMouseCommand(void);
	int WindowMapPos(int x, int y);
	void ResetKeyStates(void);
	void RestartKeystates(void);
	void KeyEventCallback(int scancode, bool down);

	// The complete array of key states.
	char keystates[TWK_LAST];

	// The last mouse action.
	mouseaction	mouseinfo;

	// TRUE if direction keys are to be treated as always repeating.
	bool joystickstyle = false;

	// A map of keys that can be held down simultaneously to produce
	// multiple commands.
	int mergeable[CmdKeyMoveLast + 1];

	void SetHint(bool newmode, QString hint = "");
	void SetScale(int s, bool checkPrevScale = true);

	bool m_bWindowClosed;

	Qt_Surface* m_pSurface;
	Qt_Surface* m_pInvSurface;
	TW_Rect m_disploc;

	double scale = 1;

	bool m_nKeyState[TWK_LAST];

	struct MessageData{ QString sMsg; uint32_t nMsgUntil, nMsgBoldUntil; };
	QVector<MessageData> m_shortMessages;

	bool m_bKbdRepeatEnabled;

	int m_nRuleset;
	int m_nLevelNum;
	QString m_sLevelName;
	QString m_sLevelPackName;
	QString m_sTimeFormat;
	bool m_bProblematic;
	bool m_bOFNT;
	int m_nBestTime;
	bool m_hintVisible;
	int m_nTimeLeft;
	bool m_bTimedLevel;
	bool m_bReplay;

	QSortFilterProxyModel* m_pSortFilterProxyModel;
	QLocale m_locale;

	CCX::Levelset m_ccxLevelset;

	QString m_sTextToCopy;

	QIcon playIcon;
	QIcon pauseIcon;

	QTimer *volTimer;

	QStringList stepDialogOptions = {
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
	QStringList subtitlestack;
};

extern TileWorldMainWnd* g_pMainWnd;

#endif
