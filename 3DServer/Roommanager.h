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


	float					m_timer;		//�ð�	���ӽ����ϸ� 600(10��)�� ����Ѵ�.
	float					m_Occupytimer;	//Ư������ ���ʵ��� �����ߴ���.
	bool					m_firstOccupy = true;



	BYTE					m_Occupy;		//���� �����ߴ���


	vector<unsigned int>	m_room_id;	//�濡 ���ִ� �÷��̾� �ִ� 8�� ���� �ؾ��Ѵ�.

	void insert_Player(int id);
	bool CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	void update();	//�ð��� 1�� ��������.
	void Killupdate(BYTE team);
	void Release();

};

