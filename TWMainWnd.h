/* Copyright (C) 2001-2019 by Madhav Shanbhag, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWMAINWND_H
#define TWMAINWND_H

#include <QMainWindow>
#include <QLocale>
#include "ui_TWMainWnd.h"
#include "CCMetaData.h"
#include "generic.h"
#include "defs.h"
#include "state.h"
#include "series.h"
#include "oshw.h"

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
		int		hold;		/* TRUE for repeating joystick-mode keys */
	} keycmdmap;

	/* The complete list of key commands recognized by the game while
	 * playing. hold is TRUE for keys that are to be forced to repeat.
	 * shift, ctl and alt are positive if the key must be down, zero if
	 * the key must be up, or negative if it doesn't matter.
	 */
	static keycmdmap constexpr keycmds[] = {
		{	TWK_UP,					CmdNorth,				TRUE	},
		{	TWK_LEFT,				CmdWest,				TRUE	},
		{	TWK_DOWN,				CmdSouth,				TRUE	},
		{	TWK_RIGHT,				CmdEast,				TRUE	},
		{	TWK_RETURN,				CmdProceed,				FALSE	},

		{	TWC_SEESCORES,			CmdSeeScores,			FALSE	},
		{	TWC_SEESOLUTIONFILES,	CmdSeeSolutionFiles,	FALSE	},
		{	TWC_TIMESCLIPBOARD,		CmdTimesClipboard,		FALSE	},
		{	TWC_QUITLEVEL,			CmdQuitLevel,			FALSE	},
		{	TWC_QUIT,				CmdQuit,				FALSE	},

		{	TWC_PROCEED,			CmdProceed,				FALSE	},
		{	TWC_PAUSEGAME,			CmdPauseGame,			FALSE	},
		{	TWC_SAMELEVEL,			CmdSameLevel,			FALSE	},
		{	TWC_NEXTLEVEL,			CmdNextLevel,			FALSE	},
		{	TWC_PREVLEVEL,			CmdPrevLevel,			FALSE	},
		{	TWC_GOTOLEVEL,			CmdGotoLevel,			FALSE	},

		{	TWC_PLAYBACK,			CmdPlayback,			FALSE	},
		{	TWC_CHECKSOLUTION,		CmdCheckSolution,		FALSE	},
		{	TWC_DELSOLUTION,		CmdDelSolution,			FALSE	},
		{	TWC_SEEK,				CmdSeek,				FALSE	},
		{	0,	0,	0	}
	};

	TileWorldMainWnd(QWidget* pParent = 0, Qt::WindowFlags flags = 0);
	~TileWorldMainWnd();

	virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	virtual void closeEvent(QCloseEvent* pCloseEvent) override;

	bool SetKeyboardRepeat(bool bEnable);
	int GetReplaySecondsToSkip() const;

	bool CreateGameDisplay();
	void ClearDisplay();
	bool DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime);
	int DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted);
	int DisplayList(const tablespec* pTableSpec, int* pnIndex, DisplayListType eListType);
	bool DisplayYesNoPrompt(const char* prompt);
	QString DisplayPasswordPrompt();
	int GetSelectedRuleset();
	void SetSubtitle(const char* szSubtitle);

	void ReadExtensions(gameseries* pSeries);
	void Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce = false);

	void ShowAbout();
	void SetPlayPauseButton(int p);
	int Input(int wait);
	int SetKeyboardArrowsRepeat(int enable);

public slots:
	void HideVolumeWidget();

private slots:
	void OnListItemActivated(const QModelIndex& index);
	void OnFindTextChanged(const QString& sText);
	void OnFindReturnPressed();
	void OnRulesetSwitched(bool mschecked);
	void OnPlayback();
	void OnSpeedValueChanged(int nValue);
	void OnSpeedSliderReleased();
	void OnSeekPosChanged(int nValue);
	void OnTextNext();
	void OnTextPrev();
	void OnTextReturn();
	void OnCopyText();
	void OnMenuActionTriggered(QAction* pAction);

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
	void KeyEventCallback(int scancode, int down);
	void MouseEventCallback(int xpos, int ypos, int button);

	// The complete array of key states.
	char keystates[TWK_LAST];

	// The last mouse action.
	mouseaction	mouseinfo;

	// TRUE if direction keys are to be treated as always repeating.
	int		joystickstyle = FALSE;

	// A map of keys that can be held down simultaneously to produce
	// multiple commands.
	int mergeable[CmdKeyMoveLast + 1];

	enum HintMode { HINT_EMPTY, HINT_TEXT, HINT_INITSTATE };
	bool SetHintMode(HintMode newmode);
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
	QString m_nLevelName;
	QString m_nLevelPackName;
	bool m_bProblematic;
	bool m_bOFNT;
	int m_nBestTime;
	HintMode m_hintMode;
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
};


#endif
