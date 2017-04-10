#pragma once


#pragma pack(push, 1)
struct cs_key_input {	//키버튼 받았을때

	BYTE	size;
	BYTE	type;

	float fDistance;

	DWORD key_button;

};

struct cs_rotate {	//화면을 움직였을때

	BYTE	size;
	BYTE	type;

	float cx;
	float cy;
	float cz;

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

struct sc_rotate_vector
{
	BYTE size;
	BYTE type;
	WORD id;

	float x;
	float y;
	float z;

};

struct sc_packet_remove_player {
	BYTE size;
	BYTE type;
	WORD id;
};
#pragma pack(pop)