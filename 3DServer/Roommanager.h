#pragma once
class CRoommanager
{
public:
	CRoommanager();
	~CRoommanager();

	int 					m_roomnumber;	//���ȣ
	vector<CServerPlayer>	m_room_player;	//�濡 ���ִ� �÷��̾� �ִ� 8�� ���� �ؾ��Ѵ�.

	void insert_Player(CServerPlayer player);

};

