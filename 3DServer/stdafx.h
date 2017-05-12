#pragma once

#include<WinSock2.h>
#include<Windows.h>
#include<iostream>
#include<thread>
#include<mutex>
#include<list>
#include<set>
#include<queue>
#include<vector>
using namespace std;

#include <D3DX10Math.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#pragma comment(lib,"Ws2_32")

using namespace DirectX;
using namespace DirectX::PackedVector;

// ----- User Header ----- //
#include "EnumDefine.h"
#include "CollisionManager.h"
#include "ServerPlayer.h"

#define SERVERPORT		9000

#define OP_RECV				1
#define OP_SEND				2
#define OP_MOVE				3

#define	SC_POS				1
#define	SC_PUT_PLAYER		2
#define	SC_PUT_Bullet		3
#define	SC_REMOVE_PLAYER	4
#define	SC_ROTATE			5
#define	SC_ColliSion		6
#define SC_PUT_HP			7

#define MAX_USER			10

#define MAX_BUFFSIZE		4000
#define COLLISION_MGR CCollisionManager::GetInstance()

struct vector3 {
	float x;
	float y;
	float z;


	vector3(int x, int y, int z) :x(x), y(y), z(z) {  };
	vector3() {};
	vector3 operator * (float f)
	{
		return vector3(f * x, f*y, f * z);
	}

};

inline void ShowXMVector(XMVECTOR xmVector)
{
	XMFLOAT4 out;
	XMStoreFloat4(&out, xmVector);
	cout << out.x << ", " << out.y << ", " << out.z << ", " << out.w << endl;
}

inline void ShowXMMatrix(XMMATRIX& mtx)
{
	XMFLOAT4X4 mtxOut;
	XMStoreFloat4x4(&mtxOut, mtx);
	cout << mtxOut._11 << ", " << mtxOut._12 << ", " << mtxOut._13 << ", " << mtxOut._14 << endl;
	cout << mtxOut._21 << ", " << mtxOut._22 << ", " << mtxOut._23 << ", " << mtxOut._24 << endl;
	cout << mtxOut._31 << ", " << mtxOut._32 << ", " << mtxOut._33 << ", " << mtxOut._34 << endl;
	cout << mtxOut._41 << ", " << mtxOut._42 << ", " << mtxOut._43 << ", " << mtxOut._44 << endl << endl;
}

inline void ShowXMFloat4x4(XMFLOAT4X4& mtx)
{
	cout << mtx._11 << ", " << mtx._12 << ", " << mtx._13 << ", " << mtx._14 << endl;
	cout << mtx._21 << ", " << mtx._22 << ", " << mtx._23 << ", " << mtx._24 << endl;
	cout << mtx._31 << ", " << mtx._32 << ", " << mtx._33 << ", " << mtx._34 << endl;
	cout << mtx._41 << ", " << mtx._42 << ", " << mtx._43 << ", " << mtx._44 << endl << endl;
}

inline void ShowXMFloat4(XMFLOAT4& xmf4)
{
	cout << xmf4.x << ", " << xmf4.y << ", " << xmf4.z << ", " << xmf4.w << endl;
}

inline void ShowXMFloat3(XMFLOAT3& xmf3)
{
	cout << xmf3.x << ", " << xmf3.y << ", " << xmf3.z << endl;
}

struct PLAYER {		//플레이어 좌표.

	float x;
	float y;
	float z;

	bool fire;

	XMFLOAT3	Animation;


	BYTE Hp;

	DWORD button;
	vector3 lookvector;

	XMFLOAT3 FireDirecton;
};


struct Event_timer {		//타이머

	int obj_id;
	unsigned wakeup_time;
	int event_id;

};

class mycomparison
{
	bool reserve;
public:
	bool operator() (const Event_timer lhs, const Event_timer rhs) const
	{
		return (lhs.wakeup_time > rhs.wakeup_time);
	}

};

struct Overlapex {
	WSAOVERLAPPED	original_overlap;
	int				operation;
	WSABUF			recv_buffer;
	BYTE			socket_buff[MAX_BUFFSIZE];
	int				packet_size;

};

struct CLIENT {
	//int				id;
	bool			connected;
	SOCKET			sock;
	Overlapex		recv_overlap;
	int				previous_data_size;
	mutex			vl_lock;
	BYTE			packet[MAX_BUFFSIZE];
	bool			Team = false;
	CServerPlayer	player;
};