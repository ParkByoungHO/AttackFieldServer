#pragma once

#define CS_KEY_TYPE		1
#define CS_ROTATE		2


#pragma pack(push, 1)
struct cs_key_input {	//키버튼 받았을때 

	BYTE	size;
	BYTE	type;

	XMFLOAT3	Animation = XMFLOAT3(0,0,0);


	DWORD	key_button;

	BYTE	 Hp;

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

	XMFLOAT3 Animation;
	BYTE hp;
};

struct sc_packet_pos	//서버에서 처리된 값을 클라에게 보낸다. 
{
	BYTE size;
	BYTE type;

	WORD id;
	

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

struct sc_packet_remove_player {	//접속이 종료되면 보내는 패킷이다.
	BYTE size;
	BYTE type;
	WORD id;
};
#pragma pack(pop)