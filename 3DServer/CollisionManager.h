#pragma once
#include "SingletonManager.h"
#include "Object.h"
#include "CharacterObject.h"

class CCollisionManager : public CSingletonManager<CCollisionManager>
{
public:
	vector<CGameObject*>				m_vecStaticMeshContainer;
	vector<CGameObject*>				m_vecDynamicMeshContainer;
	vector<CCharacterObject*>			m_vecCharacterContainer;

public:
	CCollisionManager() {};
	virtual ~CCollisionManager() {};

	virtual void InitializeManager() override;
	virtual void UpdateManager() override;
	virtual void ReleseManager() override;


	bool RayCastCollision(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	bool RayCastCollisionToCharacter(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	bool AABBCollision(CollisionInfo& info, BoundingBox bcBox);
	bool OBBCollision(CollisionInfo& info, BoundingOrientedBox bcObbox);

	// User Define
	bool CheckCollision(CollisionInfo& info, CGameObject* pObjectA, CGameObject* pObjectB);

private:
	bool RayCastCollision_AABB(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	bool RayCastCollisionToCharacter_Sphere(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
	bool RayCastCollisionToCharacter_AABB(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);
//	bool RayCastCollisionToCharacter_Parts(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);	클라에서 작동
//	bool RayCastCollisionInPolygon(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction);	 클라에서 작동
};