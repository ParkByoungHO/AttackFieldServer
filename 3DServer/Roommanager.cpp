#include "stdafx.h"
#include "Roommanager.h"


CRoommanager::CRoommanager()
{
	m_Goalkill = 0;
}


CRoommanager::~CRoommanager()
{
}


void CRoommanager::insert_Player(CLIENT &player)
{

	m_room_player.push_back(player);

}