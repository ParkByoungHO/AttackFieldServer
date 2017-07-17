#pragma once
enum Mode
{
	None,
	Death, occupy

};

class CRoommanager
{
public:
	CRoommanager(Mode mode);
	CRoommanager();
	~CRoommanager();

	BYTE					m_BlueKill;
	BYTE					m_RedKill;
	BYTE					m_Goalkill;
	Mode					m_Gamemode;


	int						m_timer;	//시간	게임시작하면 600(10분)을 줘야한다.

	BYTE					m_Occupy;	//누가 점령했는지


	vector<CLIENT *>		m_room_player;	//방에 들어가있는 플레이어 최대 8명만 들어가게 해야한다.

	void insert_Player(CLIENT *player);
	bool CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	void update();	//시간이 1씩 없어진다.
	void Killupdate(BYTE team);

};

