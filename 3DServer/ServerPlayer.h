#pragma once

#define DIR_FORWARD		0x01
#define DIR_BACKWARD	0x02
#define DIR_LEFT		0x04
#define DIR_RIGHT		0x08
//#define DIR_UP		0x10
//#define DIR_DOWN		0x20

struct PLAYER_MOVE_INFO {

	bool isMove;

	DWORD dwDirection = 0;

	XMFLOAT3					m_d3dxvPosition;
	XMFLOAT3					m_d3dxvRight;
	XMFLOAT3					m_d3dxvUp;
	XMFLOAT3					m_d3dxvLook;

	XMFLOAT3					m_d3dxvVelocity;	//이값을 클라한테 넘겨야 한다.
	XMFLOAT3     				m_d3dxvGravity;

	XMFLOAT3					Animation;
	XMFLOAT3					FireDirection;


	float           			m_fMaxVelocityXZ = 500.0f;
	float           			m_fMaxVelocityY;
	float           			m_fFriction;

	float           			m_fPitch;
	float           			m_fYaw;
	float           			m_fRoll;
	float						m_fSpeed = 0.5;

	float						m_cxDelta;
	float						m_cyDelta;

	float						m_fDistance;
};


class CServerPlayer
{
private:
	WORD			 m_wKeyState = 0;
	PLAYER_MOVE_INFO player_move_info;

	XMMATRIX	m_mtxWorld = XMMatrixIdentity();

	BYTE m_HP = 100;
	int  m_id = 0;
	bool m_fire = false;
	
public:
	CServerPlayer();
	~CServerPlayer();
	
	//void Move(DWORD nDirection, float fDistance, bool bVelocity = false);
	//void Move(const XMVECTOR& d3dxvShift, bool bVelocity = false);

	void Move(XMVECTOR d3dxvShift);
	void UpdateKeyInput(float fTimeElapsed);

	void Rotate(float x, float y);
	void Update(float fTimeElapsed);
	float getfPitch() { return player_move_info.m_fPitch; }
	float getYaw() { return player_move_info.m_fYaw; }
	void Setkey(DWORD key) { m_wKeyState = key;  UpdateKeyInput(0.15); }
	void setfire(bool fire) { m_fire = fire; }


	BYTE GetHp() { return m_HP; }
	void SetHp(BYTE hp) { m_HP = hp; }

	bool Getfire() { return m_fire; }
	
	void SetAnimation(XMFLOAT3 Animation) { player_move_info.Animation = Animation; }
	XMFLOAT3 GetAnimation() { return player_move_info.Animation; }
	void SetPosition(XMFLOAT3 Position) { player_move_info.m_d3dxvPosition = Position; }

	void SetFireDirection(XMFLOAT3	fireDirection) { player_move_info.FireDirection = fireDirection; }
	XMFLOAT3 GetFireDirection() { return player_move_info.FireDirection; }




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
			player_move_info.m_d3dxvPosition.x = 60;
			player_move_info.m_d3dxvPosition.y = 2;
			player_move_info.m_d3dxvPosition.z = 20;
		}
	}

	XMFLOAT3 Getd3dxvVelocity()
	{
		return player_move_info.m_d3dxvVelocity;
	}
	

	XMFLOAT3 GetPosition()
	{
		return player_move_info.m_d3dxvPosition;
	}

	void Setd3dxvVelocity(float x, float y, float z)
	{
		player_move_info.m_d3dxvVelocity.x = x;
		player_move_info.m_d3dxvVelocity.y = y;
		player_move_info.m_d3dxvVelocity.z = z;
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
		return player_move_info.m_d3dxvLook;
	}

};

