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
	m_timer = 600;
	m_Gamemode = mode;

}


CRoommanager::~CRoommanager()
{
}


void CRoommanager::insert_Player(CLIENT *player)
{

	m_room_player.push_back(player);	//�̺κ�

}

bool CRoommanager::CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	return COLLISION_MGR->RayCastCollisionToCharacter(info, originPos, direction);
	
}



void CRoommanager:: update()
{
	if(m_timer > 0 )
		m_timer--;
	else
	{
		// �ð� �ٵǸ� ����.
	}


}

void CRoommanager::Killupdate(BYTE team)
{
	if (team == 2)
		m_RedKill++;
	else
		m_BlueKill++;

}