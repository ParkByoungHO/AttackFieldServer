#pragma once


#include <D3DX10Math.h>


#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#define DIR_FORWARD		0x01
#define DIR_BACKWARD	0x02
#define DIR_LEFT		0x04
#define DIR_RIGHT		0x08
//#define DIR_UP		0x10
//#define DIR_DOWN		0x20


using namespace DirectX;
using namespace DirectX::PackedVector;

struct PLAYER_INPUT_INFO {

	bool key_W_Down = false;
	bool key_A_Down = false;
	bool key_S_Down = false;
	bool key_D_Down = false;

	bool key_LSHIFT_Down = false;

	bool key_1_Down = false;
	bool key_2_Down = false;
	bool key_3_Down = false;
	bool key_4_Down = false;

	bool key_SPACE_Down = false;

	bool mouse_LBUTTON_Down = false;
	bool mouse_RBUTTON_Down = false;


};


struct PLAYER_MOVE_INFO {

	bool isMove;

	DWORD dwDirection = 0;

	XMFLOAT3					m_d3dxvPosition;
	XMFLOAT3					m_d3dxvRight;
	XMFLOAT3					m_d3dxvUp;
	XMFLOAT3					m_d3dxvLook;

	XMFLOAT3					m_d3dxvVelocity;
	XMFLOAT3     				m_d3dxvGravity;
	float           			m_fMaxVelocityXZ;
	float           			m_fMaxVelocityY;
	float           			m_fFriction;

	float           			m_fPitch;
	float           			m_fYaw;
	float           			m_fRoll;

	float m_cxDelta;
	float m_cyDelta;

	float m_fDistance;

};


class CServerPlayer
{
private:
	PLAYER_INPUT_INFO player_input_info;
	PLAYER_MOVE_INFO player_move_info;

	XMMATRIX	m_mtxWorld = XMMatrixIdentity();

	int m_id;
	
	
public:
	CServerPlayer();
	~CServerPlayer();
	
	void Move(DWORD nDirection, float fDistance, bool bVelocity = false);
	void Move(const XMVECTOR& d3dxvShift, bool bVelocity = false);
	void Rotate(float x, float y, float z);
	void Update(float fTimeElapsed);
	void setid(int id) { 
		m_id = id; 

		if (m_id % 2 == 0)
		{
			player_move_info.m_d3dxvPosition.x = 65;
			player_move_info.m_d3dxvPosition.y = 2;
			player_move_info.m_d3dxvPosition.z = 12;
		}
		else
		{
			player_move_info.m_d3dxvPosition.x = 265;
			player_move_info.m_d3dxvPosition.y = 2;
			player_move_info.m_d3dxvPosition.z = 230;
		}
	}

	

	XMFLOAT3 GetPosition()
	{
		return player_move_info.m_d3dxvPosition;
	}

	XMVECTOR& GetLook()
	{
		//std::cout << player_move_info.m_d3dxvLook.x << " " << player_move_info.m_d3dxvLook.z << std::endl;
		auto m_xmVector = XMVectorSet(player_move_info.m_d3dxvLook.x, player_move_info.m_d3dxvLook.y, player_move_info.m_d3dxvLook.z, 0.0f);
		m_xmVector = XMVector3Normalize(m_xmVector);
		return(m_xmVector);
	}

	XMFLOAT3& GetLookvector()
	{
		XMFLOAT3 temp;
		XMStoreFloat3(&temp, GetLook());
		return temp;
	}

	XMMATRIX& GetMatrix() { return m_mtxWorld; }
};

