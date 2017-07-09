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

struct cs_key_input {	//키버튼 받았을때 

	BYTE	size;
	BYTE	type;

	XMFLOAT3	Animation = XMFLOAT3(0,0,0);


	WORD	key_button;

	//보여주기 식
	float	x;
	float	y;
	float	z;

	XMFLOAT3 FireDirection;


};

struct cs_rotate {	//클라에서 화면을 움직였을때 

	BYTE	size;
	BYTE	type;

	float cx;
	float cy;

};

struct sc_packet_put_player {	//서버에서 처음 접속했을때 위치값과 ID를 부여한다.

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

	//총 35Byte;
};

struct sc_packet_pos	//서버에서 처리된 값을 클라에게 보낸다. 
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

struct sc_rotate_vector	//처리된 lookvector를 보낸다.
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

struct sc_packet_remove_player {	//접속이 종료되면 보내는 패킷이다.
	BYTE size;
	BYTE type;
	WORD id;
};

struct cs_weapon {	//클라에서 무기의 정보를 보낸다.
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

	//FLOAT Time;	//나중에 시간 넣어서 보내준다.


};

struct SC_Starting_Timer
{
	BYTE size;
	BYTE type;

	float Starting_timer;

};

struct SC_Respawn	//리스폰 5초뒤에 보내줘야 하는것.
{
	BYTE size;
	BYTE type;
	BYTE id;

	BOOL m_bIsRespawn;
	XMFLOAT3 m_f3Position;
};

#pragma pack(pop)