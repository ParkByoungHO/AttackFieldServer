#pragma once
class CRoommanager
{
public:
	CRoommanager();
	~CRoommanager();

	BYTE					m_BlueKill;
	BYTE					m_RedKill;
	BYTE					m_Goalkill;

	int						m_timer;	//�ð�

	BYTE					m_Occupy;	//���� �����ߴ���


	vector<CLIENT *>		m_room_player;	//�濡 ���ִ� �÷��̾� �ִ� 8�� ���� �ؾ��Ѵ�.

	void insert_Player(CLIENT *player);
	bool CollisonCheck(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);

};

