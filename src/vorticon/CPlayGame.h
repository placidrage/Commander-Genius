/*
 * CPlayGame.h
 *
 *  Created on: 03.10.2009
 *      Author: gerstrong
 *
 */


#ifndef CPlayGame_H_
#define CPlayGame_H_

#include "../graphics/CGfxEngine.h"
#include "../common/CPlayer.h"
#include "../common/CObject.h"
#include "../common/CMenu.h"
#include "../common/CMap.h"
#include "../common/options.h"
#include <SDL/SDL.h>
#include <string>
#include <vector>

#define DOOR_YELLOW        2
#define DOOR_RED           3
#define DOOR_GREEN         4
#define DOOR_BLUE          5

#define WORLD_MAP_LEVEL 80

class CPlayGame {
public:

enum e_levelcommands
{
	NONE,
	GOTO_WORLD_MAP,
	START_LEVEL,
};

	CPlayGame( char episode, char level, 
			char numplayers, char difficulty,
			std::string &gamepath, stOption *p_option );

	bool init();
	bool loadGameState( std::string &statefile );

	void process();
	void drawObjects();
	bool scrollTriggers();

	// Collision stuff stored in CPlayGameCollision.cpp
	void checkPlayerCollisions(CPlayer *p_player);
	bool checkisSolidr(CPlayer *p_player);
	bool checkisSolidl(CPlayer *p_player);
	bool checkisSolidd(CPlayer *p_player);
	bool checkisSolidu(CPlayer *p_player);
	char checkDoorBlock(int t, CPlayer *p_player, int which);
	int checkObjSolid(unsigned int x, unsigned int y, int cp);
	void processPlayerfallings(CPlayer *p_player);

	// Dialog processes stored in CPlayGameDialogs.cpp
	void processPauseDialogs();
	void showPausedGameDlg();

	bool isFinished() 
		{ return m_finished; }

	bool getEndGame() { return m_endgame; }
	bool getExitEvent() { return m_exitgame; }

	void cleanup();
	virtual ~CPlayGame();

private:
	bool m_finished;
	bool m_endgame;
	bool m_exitgame;
	int  m_NumSprites;
	char m_Episode;
	char m_Level;
	char m_NumPlayers;
	char m_Difficulty;
	char m_level_command;
	bool m_paused;
	bool m_showPauseDialog;
	std::string m_Gamepath;
	bool mp_level_completed[16];

	CMap *mp_Map;
	CMenu *mp_Menu;
	CPlayer *mp_Player;
	stOption *mp_option;
	int m_theplayer;
	std::vector<CObject> m_Object;
};
#endif /* CPlayGame_H_ */
