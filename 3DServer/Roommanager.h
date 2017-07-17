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


	int						m_timer;	//�ð�	���ӽ����ϸ� 600(10��)�� ����Ѵ�.

	BYTE					m_Occupy;	//���� �����ߴ���


	vector<CLIENT *>		m_room_player;	//�濡 ���ִ� �÷��̾� �ִ� 8�� ���� �ؾ��Ѵ�.

	void insert_Player(CLIENT *player);
	bool CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	void update();	//�ð��� 1�� ��������.
	void Killupdate(BYTE team);

};

