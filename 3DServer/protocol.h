#pragma once
#pragma pack(push, 1)

struct cs_key_input {	//Ű��ư �޾�����

	BYTE	size;
	BYTE	type;

	float x;
	float y;
	float z;

	float fDistance;

	DWORD key_button;

};

struct cs_rotate {	//ȭ���� ����������

	BYTE	size;
	BYTE	type;

	//BYTE	key_input;

	float cx;
	float cy;
	float cz;

	//float x;
	//float y;
	//float z;

};

struct sc_packet_put_player {	

	BYTE size;
	BYTE type;

	WORD id;

	int x;
	int y;
	int z;

};

struct sc_packet_pos
{
	BYTE size;
	BYTE type;


	WORD id;
	int x;
	int y;
	int z;
};
#pragma pack(pop)