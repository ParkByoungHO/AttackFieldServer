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

	mutex					lock;

	BYTE					m_BlueKill;
	BYTE					m_RedKill;
	BYTE					m_Goalkill;
	Mode					m_Gamemode;


	float					m_timer;		//시간	게임시작하면 600(10분)을 줘야한다.
	float					m_Occupytimer;	//특정팀이 몇초동안 점령했는지.
	bool					m_firstOccupy = true;



	BYTE					m_Occupy;		//누가 점령했는지


	vector<unsigned int>	m_room_id;	//방에 들어가있는 플레이어 최대 8명만 들어가게 해야한다.

	void insert_Player(int id);
	bool CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	void update();	//시간이 1씩 없어진다.
	void Killupdate(BYTE team);
	void Release();

};

