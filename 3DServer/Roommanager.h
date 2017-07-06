#pragma once
class CRoommanager
{
public:
	CRoommanager();
	~CRoommanager();

	int 					m_roomnumber;	//방번호
	vector<CServerPlayer>	m_room_player;	//방에 들어가있는 플레이어 최대 8명만 들어가게 해야한다.

	void insert_Player(CServerPlayer player);

};

