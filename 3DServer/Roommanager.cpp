#include "stdafx.h"
#include "Roommanager.h"

CRoommanager::CRoommanager()
{

}
CRoommanager::CRoommanager(Mode mode)
{
	m_RedKill = 0;
	m_BlueKill =0;
	m_Goalkill = 50;
	m_timer = 610;
	m_Occupytimer = 0;
	m_Gamemode = mode;

}


CRoommanager::~CRoommanager()
{
}


void CRoommanager::insert_Player(int id)
{
	lock.lock();
	m_room_id.push_back(id);	//ÀÌºÎºÐ
	lock.unlock();
}

bool CRoommanager::CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	return COLLISION_MGR->RayCastCollisionToCharacter(info, originPos, direction);
	
}

void CRoommanager::Release()
{
	lock.lock();
	m_room_id.clear();	
	m_RedKill = 0;
	m_BlueKill = 0;
	m_Goalkill = 0;
	m_timer = 0;
	m_Occupytimer = 0;
	m_Gamemode = Mode::None;
	lock.unlock();
}
void CRoommanager:: update()
{
	if(m_timer > 0 )
		m_timer-=10;

}

void CRoommanager::Killupdate(BYTE team)
{
	if (team == 2)
		m_RedKill++;
	else
		m_BlueKill++;

}