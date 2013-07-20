/*
 * CMapPlayGalaxy.cpp
 *
 *  Created on: 25.03.2011
 *      Author: gerstrong
 */

#include "CMapPlayGalaxy.h"
#include "common/ai/platform/CPlatform.h"
#include "common/ai/CPlayerBase.h"
#include "common/CBehaviorEngine.h"
#include "ep4/CMapLoaderGalaxyEp4.h"
#include "ep5/CMapLoaderGalaxyEp5.h"
#include "ep6/CMapLoaderGalaxyEp6.h"
#include "ep4/ai/CInchWorm.h"
#include "ep4/ai/CFoot.h"
#include "ep4/ai/CSmokePuff.h"
#include "CLogFile.h"

CMapPlayGalaxy::CMapPlayGalaxy(CExeFile &ExeFile, CInventory &Inventory, stCheat &Cheatmode) :
mActive(false),
mExeFile(ExeFile),
mInventory(Inventory),
mpOption(g_pBehaviorEngine->m_option),
mCheatmode(Cheatmode),
mMsgBoxOpen(false)
{}


bool CMapPlayGalaxy::isActive()
{	return mActive;	}




void CMapPlayGalaxy::setActive(bool value)
{
	mActive = value;

	if(mActive)
	{
	    mMap.drawAll();
	    g_pVideoDriver->updateScrollBuffer(mMap);
	}
}




/**
 * return the name of the opened level
 */
std::string CMapPlayGalaxy::getLevelName()
{
	return mMap.getLevelName();
}







void CMapPlayGalaxy::ponder()
{
    const bool msgboxactive = mMsgBoxOpen;

	// Check if the engine need to be paused
	const bool pause = mInventory.showStatus() || msgboxactive;

	// Animate the tiles of the map
	mMap.m_animation_enabled = !pause;
	mMap.animateAllTiles();

	if(!pause)
	{	    
	    for( auto obj = mObjectPtr.begin(); obj != mObjectPtr.end() ; obj++)
	    {
		auto &objRef = *(obj->get());	
		bool visibility = false;

		if( objRef.exists )
		{
		    visibility = objRef.calcVisibility();
		    
		    if( visibility )
		    {
			// Process the AI of the object as it's given
			objRef.process();
			
			// process all the objects' events
			objRef.processEvents();
			
			// Check collision between objects using NlogN order
			auto theOther = obj; theOther++;			
			for( ; theOther != mObjectPtr.end() ; theOther++)
			{
			    auto &theOtherRef = *(theOther->get());
			    if( !theOtherRef.exists )
                    continue;
			    
			    objRef.isNearby(theOtherRef);
                theOtherRef.isNearby(objRef);
			    
			    if( objRef.hitdetect(theOtherRef) )
			    {
                    objRef.getTouchedBy(theOtherRef);
                    theOtherRef.getTouchedBy(objRef);
			    }
			}
		    }
		}
		
		// If the Player is not only dying, but also lost it's existence, meaning he got out of the screen
		// how the death-message or go gameover.
		if( galaxy::CPlayerBase *player = dynamic_cast<galaxy::CPlayerBase*>(obj->get()) )
		{
		    // Is he really dead?
		    if( player->dead || (!visibility && player->m_dying) )
			player->processDead();
		}
		
	    }	    
	}



	CEventContainer &EventContainer = g_pBehaviorEngine->m_EventList;
	if( EventSpawnObject *ev =  EventContainer.occurredEvent<EventSpawnObject>() )
	{
	    std::shared_ptr<CGalaxySpriteObject> obj( static_cast<CGalaxySpriteObject*>(
						    const_cast<CSpriteObject*>(ev->pObject) ) );
		mObjectPtr.push_back( move(obj) );
		EventContainer.pop_Event();
	}

	// Special Case where the Foot is created
	if( EventSpawnFoot *ev =  EventContainer.occurredEvent<EventSpawnFoot>() )
	{
		// kill all the InchWorms in that case, so they can't do any spawning
        for( auto obj=mObjectPtr.rbegin() ;
                  obj!=mObjectPtr.rend() ;
                  obj++ )
		{
			galaxy::CInchWorm *inchworm = dynamic_cast<galaxy::CInchWorm*>(obj->get());
			if( inchworm != NULL )
			{
				inchworm->exists = false;
			}
		}


		// Create the foot with Smoke Puff
		int posX = ev->x;
		int posY = ev->y-(4<<CSF);
		for( int x=-1 ; x<2 ; x++ )
		{
		    for( int y=-1 ; y<2 ; y++ )
		    {
			std::shared_ptr<CGalaxySpriteObject> smoke(new galaxy::CSmokePuff( &mMap, posX+(x<<CSF), posY+(y<<CSF) ));
			mObjectPtr.push_back( smoke );
		    }
		}
			
		std::shared_ptr<CGalaxySpriteObject> foot(new galaxy::CFoot( &mMap, ev->foeID, 0x2EF4, posX, posY));
		mObjectPtr.push_back( foot );

		// Flush all the pending events. This help catch cases when more than one more of the worms try to create the foot
		EventContainer.clear();
	}

}

void CMapPlayGalaxy::render()
{
    g_pVideoDriver->blitScrollSurface();

    auto obj = mObjectPtr.rbegin();

    // Draw all the sprites without player
    // The player sprites are drawn as last
    galaxy::CPlayerBase *player = NULL;
    for( ; obj!=mObjectPtr.rend() ; obj++ )
    {

        if( galaxy::CPlayerBase* curplayer = dynamic_cast<galaxy::CPlayerBase*>(obj->get()) )
        {
            player = curplayer;
            continue;
        }

        if((*obj)->honorPriority )
            (*obj)->draw();
    }

    if(player)
    {
        player->draw();
    }

    // Draw masked tiles here!
    mMap._drawForegroundTiles();

    for( obj=mObjectPtr.rbegin() ;
            obj!=mObjectPtr.rend() ; obj++ )
    {
        if(!(*obj)->honorPriority)
            (*obj)->draw();
    }

    if(mpOption[OPT_HUD].value )
        mInventory.drawHUD();

}


// Saved GameState stuff for that individual map

void CMapPlayGalaxy::operator>>(CSaveGameController &savedGame)
{
	const Uint16 level = mMap.getLevel();
	savedGame.encodeData( level );

	std::vector< std::shared_ptr<CGalaxySpriteObject> > filteredObjects;	

	// let's filter the Foe out that won't do any good!	
	for( auto &it : mObjectPtr )
	{
	    if( it->mFoeID != 0 )
	    {
		filteredObjects.push_back( it );
	    }
	}
	
	const size_t size = filteredObjects.size();
		
	// save the number of objects on screen
	savedGame.encodeData(size);

	for( auto &it : filteredObjects )
	{
		// save all the objects states
		unsigned int newYpos = it->getYPosition();
		unsigned int newXpos = it->getXPosition();	
		
		savedGame.encodeData( it->mFoeID );
		savedGame.encodeData( newXpos );
		savedGame.encodeData( newYpos );
		savedGame.encodeData( it->dead );
		savedGame.encodeData( it->onscreen );
		savedGame.encodeData( it->hasbeenonscreen );
		savedGame.encodeData( it->exists );
		savedGame.encodeData( it->blockedd );
		savedGame.encodeData( it->blockedu );
		savedGame.encodeData( it->blockedl );
		savedGame.encodeData( it->blockedr );	
		savedGame.encodeData( it->xDirection );
		savedGame.encodeData( it->yDirection );		
		savedGame.encodeData( it->mHealthPoints );
		savedGame.encodeData( it->canbezapped );
		savedGame.encodeData( it->cansupportplayer );
		savedGame.encodeData( it->inhibitfall );
		savedGame.encodeData( it->honorPriority );
		savedGame.encodeData( it->sprite );
		savedGame.encodeData( it->m_ActionNumber );
		it->serialize(savedGame);
	}

	// Save the map_data as it is left
	savedGame.encodeData(mMap.m_width);
	savedGame.encodeData(mMap.m_height);

	const Uint32 mapSize = mMap.m_width*mMap.m_height*sizeof(word);

	savedGame.addData( reinterpret_cast<byte*>(mMap.getBackgroundData()), mapSize );
	savedGame.addData( reinterpret_cast<byte*>(mMap.getForegroundData()), mapSize );
	savedGame.addData( reinterpret_cast<byte*>(mMap.getInfoData()), mapSize );
}



// This is for loading the game
bool CMapPlayGalaxy::operator<<(CSaveGameController &savedGame)
{
	Uint16 level;
	savedGame.decodeData( level );

	std::unique_ptr<galaxy::CMapLoaderGalaxy> mapLoader;
	const unsigned int episode = g_pBehaviorEngine->getEpisode();

	if(episode == 4)
	{
		mapLoader.reset( new galaxy::CMapLoaderGalaxyEp4(mExeFile, mObjectPtr, mInventory, mCheatmode) );
	}
	else if(episode == 5)
	{
		mapLoader.reset( new galaxy::CMapLoaderGalaxyEp5(mExeFile, mObjectPtr, mInventory, mCheatmode) );
	}
	else if(episode == 6)
	{
		mapLoader.reset( new galaxy::CMapLoaderGalaxyEp6(mExeFile, mObjectPtr, mInventory, mCheatmode) );
	}
	else
	{
		g_pLogFile->textOut("Error loading the file. This game is not supported!");
		return false;
	}

	// Load the World map level.
	mapLoader->loadMap( mMap, level );

    // Load the Background Music
	g_pMusicPlayer->stop();

    if( !g_pMusicPlayer->load(mExeFile, level) )
    	g_pLogFile->textOut("Warning: The music cannot be played. Check that all the files have been correctly copied!");
    else
    	g_pMusicPlayer->play();


	// load the number of objects on screen
	Uint32 size;
	Uint32 x, y;
	Uint16 foeID;
	uint16_t actionNumber;
	savedGame.decodeData(size);

	// Now load the previously created objects

	if(!mObjectPtr.empty())
		mObjectPtr.clear();
	
	mMap.mNumFuses = 0;
	mMap.mFuseInLevel = false;

	for( Uint32 i=0 ; i<size ; i++ )
	{
		savedGame.decodeData(foeID);
		savedGame.decodeData(x);
		savedGame.decodeData(y);

		CGalaxySpriteObject *pNewfoe = mapLoader->addFoe(mMap, foeID, x, y);

		// TODO: Be careful here is a bad Null Pointer inside that structure
		if(pNewfoe == NULL)
		{
		    pNewfoe = new CGalaxySpriteObject(&mMap, foeID, x, y);
		}

		savedGame.decodeData( pNewfoe->dead );
		savedGame.decodeData( pNewfoe->onscreen );
		savedGame.decodeData( pNewfoe->hasbeenonscreen );
		savedGame.decodeData( pNewfoe->exists );
		savedGame.decodeData( pNewfoe->blockedd );
		savedGame.decodeData( pNewfoe->blockedu );
		savedGame.decodeData( pNewfoe->blockedl );
		savedGame.decodeData( pNewfoe->blockedr );
		savedGame.decodeData( pNewfoe->xDirection );
		savedGame.decodeData( pNewfoe->yDirection );
		savedGame.decodeData( pNewfoe->mHealthPoints );
		savedGame.decodeData( pNewfoe->canbezapped );
		savedGame.decodeData( pNewfoe->cansupportplayer );
		savedGame.decodeData( pNewfoe->inhibitfall );
		savedGame.decodeData( pNewfoe->honorPriority );
		savedGame.decodeData( pNewfoe->sprite );
		savedGame.decodeData( actionNumber );
		pNewfoe->deserialize(savedGame);

		if(pNewfoe->exists)
		{
		    pNewfoe->setActionForce(actionNumber);
		    std::shared_ptr<CGalaxySpriteObject> newFoe(pNewfoe);		    
		    mObjectPtr.push_back(newFoe);
		}
	}

	// Save the map_data as it is left
	savedGame.decodeData(mMap.m_width);
	savedGame.decodeData(mMap.m_height);

	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getBackgroundData()) );
	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getForegroundData()) );
	savedGame.readDataBlock( reinterpret_cast<byte*>(mMap.getInfoData()) );

	if( mMap.m_width * mMap.m_height > 0 )
	{
		mMap.drawAll();
	}

	return true;
}


// Saves the level using the Savegamecontroller in XML.
void CMapPlayGalaxy::operator>>(boost::property_tree::ptree &levelNode)
{

    // TODO: Coding here
    const Uint16 level = mMap.getLevel();
    levelNode.put("level", level);

    std::vector< std::shared_ptr<CGalaxySpriteObject> > filteredObjects;

    // let's filter the Foe out that won't do any good, by having an invalid foe id
    for( auto &it : mObjectPtr )
    {
        if( it->mFoeID != 0 )
        {
            filteredObjects.push_back( it );
        }
    }

    const size_t size = filteredObjects.size();

    // save the number of objects on screen
    levelNode.put("NumSprites", size);

    for( auto &it : filteredObjects )
    {
        auto &spriteNode = levelNode.add("Sprite", "");

        // save all the objects states
        unsigned int newYpos = it->getYPosition();
        unsigned int newXpos = it->getXPosition();

        spriteNode.put("<xmlattr>.id", it->mFoeID);
        spriteNode.put("<xmlattr>.x", newXpos);
        spriteNode.put("<xmlattr>.y", newYpos);

        spriteNode.put("dead", it->dead);
        spriteNode.put("onscreen", it->onscreen);
        spriteNode.put("hasbeenonscreen", it->hasbeenonscreen);
        spriteNode.put("exists", it->exists);
        spriteNode.put("blockedd", it->blockedd);
        spriteNode.put("blockedu", it->blockedu);
        spriteNode.put("blockedl", it->blockedl);
        spriteNode.put("blockedr", it->blockedr);
        spriteNode.put("xDirection", it->xDirection);
        spriteNode.put("yDirection", it->yDirection);
        spriteNode.put("health", it->mHealthPoints);
        spriteNode.put("canbezapped", it->canbezapped);
        spriteNode.put("cansupportplayer", it->cansupportplayer);
        spriteNode.put("inhibitfall", it->inhibitfall);
        spriteNode.put("honorPriority", it->honorPriority);
        spriteNode.put("spritePic", it->sprite);
        spriteNode.put("Actionumber", it->m_ActionNumber);
        it->serialize(spriteNode);
    }

    /*// Save the map_data as it is left
    savedGame.encodeData(mMap.m_width);
    savedGame.encodeData(mMap.m_height);

    const Uint32 mapSize = mMap.m_width*mMap.m_height*sizeof(word);

    savedGame.addData( reinterpret_cast<byte*>(mMap.getBackgroundData()), mapSize );
    savedGame.addData( reinterpret_cast<byte*>(mMap.getForegroundData()), mapSize );
    savedGame.addData( reinterpret_cast<byte*>(mMap.getInfoData()), mapSize );
    */
}

// This is for loading the game
void CMapPlayGalaxy::operator<<(boost::property_tree::ptree &levelNode)
{
    // TODO: Coding here
}


