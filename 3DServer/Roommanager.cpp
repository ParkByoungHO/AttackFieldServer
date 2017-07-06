#include "stdafx.h"
#include "Roommanager.h"


CRoommanager::CRoommanager()
{
}


CRoommanager::~CRoommanager()
{
}


void CRoommanager::insert_Player(CServerPlayer player)
{
	m_roomnumber += 1;
	m_room_player.push_back(player);

}