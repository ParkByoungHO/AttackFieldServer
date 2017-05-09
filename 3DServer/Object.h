#pragma once

class CGameObject;
struct CollisionInfo
{
	CGameObject*		m_pHitObject = nullptr;
	float				m_fDistance = FLT_MAX;
	XMFLOAT3			m_f3HitNormal = XMFLOAT3(0, 0, 0);
	DWORD				m_dwFaceIndex = 0;
	float				m_fU = 0.0f;
	float				m_fV = 0.0f;
	UINT				m_nObjectID = 0;
	ChracterBoundingBoxParts	m_HitParts = ChracterBoundingBoxParts::eNone;
};

class CGameObject
{
public:
	CGameObject(int nMeshes = 0);
	virtual ~CGameObject();
	
public:
	XMMATRIX						m_mtxLocal = XMMatrixIdentity();
	XMMATRIX						m_mtxWorld = XMMatrixIdentity();
	XMMATRIX						m_mtxShadow = XMMatrixIdentity();

protected:
	// ----- Identity ----- //
	static UINT						g_nObjectId;
	UINT							m_nObjectId = 0;
	// 팀 종류 구별하여 충돌 처리시 사용하기. 현재는 아무것도 없음
	TeamType						m_tagTeam = TeamType::eNone;
	bool							m_bActive = true;
	
	// ----- Collision ------ //
	bool							m_bIsCollision = false;

	BoundingBox						m_bcMeshBoundingBox;		// 카메라 컬링용 Box
	BoundingSphere					m_bsMeshBoundingSphere;
	BoundingOrientedBox				m_bcMeshBoundingOBox;
	CollisionInfo					m_infoCollision;

public:
	virtual void Update(float fDeltaTime);
	virtual void OnCollisionCheck() {};
	
	void GenerateRayForPicking(XMVECTOR *pd3dxvPickPosition, XMMATRIX *pd3dxmtxWorld, XMMATRIX *pd3dxmtxView, XMVECTOR *pd3dxvPickRayPosition, XMVECTOR *pd3dxvPickRayDirection);
	
	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);
	void Move(XMFLOAT3 vPos, bool isLocal = false);
	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f, bool isLocal = false);
	void Rotate(XMFLOAT3 fAngle, bool isLocal = false);
	void Rotate(XMVECTOR *pd3dxvAxis, float fAngle, bool isLocal = false);
	// ---------- Get, Setter ---------- //
	void SetActive(bool bActive = false) { m_bActive = bActive; }	bool GetActive() const { return m_bActive; }
	void SetPosition(float x, float y, float z, bool isLocal = false);
	void SetPosition(XMVECTOR d3dxvPosition, bool isLocal = false);
	void SetPosition(XMFLOAT3 d3dxvPosition, bool isLocal = false);
	virtual void SetRotate(float fPitch, float fYaw, float fRoll, bool isLocal = false);
	virtual void SetRotate(XMFLOAT3 fAngle, bool isLocal = false);
	virtual void SetRotate(XMVECTOR *pd3dxvAxis, float fAngle, bool isLocal = false);

	XMVECTOR GetvPosition(bool bIsLocal = false) const;
	XMFLOAT3 GetPosition(bool isLocal = false) const;
	XMVECTOR GetvLook(bool bIsLocal = false) const;
	XMVECTOR GetvUp(bool bIsLocal = false) const;
	XMVECTOR GetvRight(bool bIsLocal = false) const;
	XMFLOAT3 GetLook(bool bIsLocal = false) const;
	XMFLOAT3 GetUp(bool bIsLocal = false) const;
	XMFLOAT3 GetRight(bool bIsLocal = false) const;

	void SetLook(XMFLOAT3 axis, bool bIsLocal = false);
	void SetvLook(XMVECTOR axis, bool bIsLocal = false);
	void SetUp(XMFLOAT3 axis, bool bIsLocal = false);
	void SetRight(XMFLOAT3 axis, bool bIsLocal = false);

	BoundingBox GetBoundingBox(bool isLocal = false) const;
	BoundingSphere GetBoundingSphere(bool isLocal = false) const;
	BoundingOrientedBox GetBoundingOBox(bool isLocal = false) const;
	bool GetCollisionCheck() const { return m_bIsCollision; }
	void SetCollision(bool collision) { m_bIsCollision = collision; }
	UINT GetObjectID()const { return m_nObjectId; }
	CollisionInfo GetCollisionInfo() const { return m_infoCollision; }
};