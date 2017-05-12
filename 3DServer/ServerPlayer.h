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

	XMMATRIX		m_mtxWorld = XMMatrixIdentity();

	int				m_HP = 100;
	int				m_id = 0;
	int				m_Charid = 0;	//클라에서 컨테이너가 0으로 시작하는데 자기자신이 0으로 들어간다.
									//그래서 다른캐릭터 움직이려면 1부터 시작해야 하므로 main에서 static int를 선언해
									//접속하면 값을 계속 ++해서 넣어줘야 한다.


	bool			m_life = false;	//죽으면 true가 되고 리스폰 구현할 예정.
	bool			m_fire = false;	//총 발포

	
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
	void Setkey(DWORD key) { m_wKeyState = key;  UpdateKeyInput(0.15); Update(0.15); }
	void setfire(bool fire) { m_fire = fire; }


	int GetPlayerHp() { return m_HP; }
	void SetPlayerHp(int hp) { m_HP = hp;  m_life = false; }
	void DamegeplayerHp(int damage)
	{
		if (m_HP <= damage) {
			m_HP = 0;
			m_life = true;
		}
		else
			m_HP -= damage;
	}

	bool Getlife() { return m_life; }
	bool Getfire() { return m_fire; }
	
	void SetAnimation(XMFLOAT3 Animation) { player_move_info.Animation = Animation; }
	XMFLOAT3 GetAnimation() { return player_move_info.Animation; }
	void SetPosition(XMFLOAT3 Position) { player_move_info.m_d3dxvPosition = Position; }

	void SetFireDirection(XMFLOAT3	fireDirection) { player_move_info.FireDirection = fireDirection; }
	XMFLOAT3 GetFireDirection() { return player_move_info.FireDirection; }

	XMMATRIX GetWorldMatrix() { return m_mtxWorld; }




	void setid (int id); 
	int  Getid() const { return m_id; }

	void SetCharid(int id) { m_Charid = id; }
	int  GetCharid() const { return m_Charid; }

	XMFLOAT3 const Getd3dxvVelocity()
	{
		return player_move_info.m_d3dxvVelocity;
	}
	

	XMFLOAT3 const GetPosition()
	{
		return player_move_info.m_d3dxvPosition;
	}

	void Setd3dxvVelocity(float x, float y, float z)
	{
		player_move_info.m_d3dxvVelocity.x = x;
		player_move_info.m_d3dxvVelocity.y = y;
		player_move_info.m_d3dxvVelocity.z = z;
	}

	XMVECTOR const GetLook()
	{
		//std::cout << player_move_info.m_d3dxvLook.x << " " << player_move_info.m_d3dxvLook.z << std::endl;
		auto m_xmVector = XMVectorSet(player_move_info.m_d3dxvLook.x, player_move_info.m_d3dxvLook.y, player_move_info.m_d3dxvLook.z, 0.0f);
		m_xmVector = XMVector3Normalize(m_xmVector);
		return(m_xmVector);
	}
};

