#include "stdafx.h"
#include "Roommanager.h"

CRoommanager::CRoommanager()
{

}
CRoommanager::CRoommanager(Mode mode)
{
	m_RedKill = 0;
	m_BlueKill = 0;
	m_Goalkill = 100;
	m_timer = 608;
	m_Gamemode = mode;

}


CRoommanager::~CRoommanager()
{
}


void CRoommanager::insert_Player(CLIENT *player)
{

	m_room_player.push_back(player);	//이부분

}

bool CRoommanager::CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	return COLLISION_MGR->RayCastCollisionToCharacter(info, originPos, direction);
	
}

void CRoommanager::Release()
{
	for (auto &p : m_room_player)
	{
		p->room_num = 0;
		
	}

	m_room_player.clear();

}
void CRoommanager:: update()
{
	if(m_timer > 0 )
		m_timer--;
	else
	{
		// 시간 다되면 종료.
	}


}

void CRoommanager::Killupdate(BYTE team)
{
	if (team == 2)
		m_RedKill++;
	else
		m_BlueKill++;

}