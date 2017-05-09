#pragma once

#define CS_KEY_TYPE		1
#define CS_ROTATE		2


#pragma pack(push, 1)
struct cs_key_input {	//Ű��ư �޾����� 

	BYTE	size;
	BYTE	type;

	XMFLOAT3	Animation = XMFLOAT3(0,0,0);


	DWORD	key_button;

	BYTE	 Hp;

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

	XMFLOAT3 Animation;
	BYTE hp;
};

struct sc_packet_pos	//�������� ó���� ���� Ŭ�󿡰� ������. 
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

struct sc_packet_remove_player {	//������ ����Ǹ� ������ ��Ŷ�̴�.
	BYTE size;
	BYTE type;
	WORD id;
};
#pragma pack(pop)