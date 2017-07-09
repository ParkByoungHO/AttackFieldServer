#pragma once

#define CS_KEY_TYPE		1
#define CS_ROTATE		2
#define CS_WEAPONE		3
#define CS_HEAD_HIT		4
#define CS_GAME_MODE	5

#define Dead_Reckoning	7

#pragma pack(push, 1)

struct cs_Gamemode
{
	BYTE size;
	BYTE type;
	BYTE mode;

};

struct cs_key_input {	//Ű��ư �޾����� 

	BYTE	size;
	BYTE	type;

	XMFLOAT3	Animation = XMFLOAT3(0,0,0);


	WORD	key_button;

	//�����ֱ� ��
	float	x;
	float	y;
	float	z;

	XMFLOAT3 FireDirection;


};

struct cs_rotate {	//Ŭ�󿡼� ȭ���� ���������� 

	BYTE	size;
	BYTE	type;

	float cx;
	float cy;

};

struct sc_packet_put_player {	//�������� ó�� ���������� ��ġ���� ID�� �ο��Ѵ�.

	BYTE size;	  
	BYTE type;

	WORD id;

	float x;
	float y;
	float z;

	BYTE Team;

	XMFLOAT3 Animation;
	BYTE hp;

	BYTE Goal;
	BYTE RED;
	BYTE Blue;
	float timer;

	//�� 35Byte;
};

struct sc_packet_pos	//�������� ó���� ���� Ŭ�󿡰� ������. 
{
	BYTE size;
	BYTE type;

	WORD id;
	WORD key_button;

	float x;
	float y;
	float z;

	XMFLOAT3 Animation;
	BYTE Hp;
};

struct sc_rotate_vector	//ó���� lookvector�� ������.
{
	BYTE size;
	BYTE type;

	WORD id;

	float x;
	float y;
	float z;
};

struct sc_bullet_fire
{
	BYTE size;
	BYTE type;

	WORD id;

	bool fire;
	
	XMFLOAT3 FireDirection;
};

struct sc_Reload
{
	BYTE size;
	BYTE type;

	WORD id;

	bool reload;

};

struct SC_Run
{
	BYTE size;
	BYTE type;

	WORD id;

	bool Run;


};

struct sc_packet_remove_player {	//������ ����Ǹ� ������ ��Ŷ�̴�.
	BYTE size;
	BYTE type;
	WORD id;
};

struct cs_weapon {	//Ŭ�󿡼� ������ ������ ������.
	BYTE size;
	BYTE type;

	XMFLOAT3 direction;
	XMFLOAT3 position;

};

struct SC_Collison
{
	BYTE size;
	BYTE type;

	WORD id;

	BOOL collision;
	XMFLOAT3 position;
	XMFLOAT3 direction;
};

struct CS_Head_Collison
{
	BYTE size;
	BYTE type;

	BOOL Head;

	BYTE id;
};

struct SC_Player_Hp
{
	BYTE size;
	BYTE type;

	BYTE Hp;
	BYTE id;

	BOOL Head;
	BOOL live;
};

struct SC_System_kill
{
	BYTE size;
	BYTE type;

	BYTE RED;
	BYTE BLUE;

	//FLOAT Time;	//���߿� �ð� �־ �����ش�.


};

struct SC_Starting_Timer
{
	BYTE size;
	BYTE type;

	float Starting_timer;

};

struct SC_Respawn	//������ 5�ʵڿ� ������� �ϴ°�.
{
	BYTE size;
	BYTE type;
	BYTE id;

	BOOL m_bIsRespawn;
	XMFLOAT3 m_f3Position;
};

#pragma pack(pop)