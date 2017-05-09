#include "stdafx.h"
#include "CollisionManager.h"

void CCollisionManager::InitializeManager()
{
}

void CCollisionManager::ReleseManager()
{
}

void CCollisionManager::UpdateManager()
{
	/*
	for (auto& dynamicObject : m_vecDynamicMeshContainer) {
		if (dynamicObject->GetActive()) {
			// Dynamic to Dynamic
			for (auto& targetDynamicObject : m_vecDynamicMeshContainer) {
				if (dynamicObject == targetDynamicObject)
					continue; 

				if (targetDynamicObject->GetActive()) {

		//		dynamicObject->IsCollision(targetDynamicObject);
				}
			}

			// Dynamic to Static
			for (auto& staticObject : m_vecStaticMeshContainer) {
				if (dynamicObject->IsCollision(staticObject)) {
					dynamicObject->SetActive(false);
					staticObject->SetCollision(true);
				}

			}
		}
	}
	*/
}

bool CCollisionManager::RayCastCollision(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	bool isCollision = false;
	float fNearestDistance = FLT_MAX;
	CGameObject* pNearestObject = nullptr;
	// Dynamic to Dynamic

	// Dynamic to Static
	for (auto& staticObject : m_vecStaticMeshContainer) {	
		if (staticObject->GetBoundingOBox().Intersects(originPos, direction, info.m_fDistance)) {
			if (0 < info.m_fDistance && info.m_fDistance < fNearestDistance) {
				fNearestDistance = info.m_fDistance;
				pNearestObject = staticObject;
				isCollision = true;
			}
		}
	}

	if (isCollision) {
		info.m_pHitObject = pNearestObject;
		info.m_fDistance = fNearestDistance;
		return true;
	}

	return false;
}

bool CCollisionManager::RayCastCollisionToCharacter(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)	//캐릭터 충돌 함수.
{
	
	// 1차 Sphere
	if (!RayCastCollisionToCharacter_Sphere(info, originPos, direction))	//이거먼저
		return false;
		
	// 2차 AABB
	if (!RayCastCollisionToCharacter_AABB(info, originPos, direction))		//aabb박스로
		return false;

	return true;
}

bool CCollisionManager::RayCastCollision_AABB(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	CGameObject* pNearestObject = nullptr;
	float fNearestDistance = FLT_MAX;
	float fDist = 0;
	bool isCollision = false;

	for (auto& object : m_vecStaticMeshContainer) {
		if (object->GetBoundingBox(0).Intersects(originPos, direction, info.m_fDistance)) {
			if (fNearestDistance > info.m_fDistance) {
				fNearestDistance = info.m_fDistance;
				pNearestObject = object;
				isCollision = true;
				cout << "1차 충돌 " << endl;
			}
		}
	}

	if (isCollision) {
		info.m_pHitObject = pNearestObject;
		return true;
	}
	return false;
}

bool CCollisionManager::RayCastCollisionToCharacter_Sphere(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	CGameObject* pNearestObject = nullptr;
	float fNearestDistance = FLT_MAX;
	float fDist = 0;
	bool isCollision = false;

	for (auto& character : m_vecCharacterContainer) {
		if (character->GetBoundingSphere(0).Intersects(originPos, direction, info.m_fDistance)) {
			if (fNearestDistance > info.m_fDistance) {
				fNearestDistance = info.m_fDistance;
				pNearestObject = character;
				isCollision = true;
			}
		}
	}

	if (isCollision) {
		info.m_pHitObject = pNearestObject;
		return true;
	}
	return false;
}

bool CCollisionManager::RayCastCollisionToCharacter_AABB(CollisionInfo& info, XMVECTOR originPos, XMVECTOR direction)
{
	if (info.m_pHitObject->GetBoundingBox(0).Intersects(originPos, direction, info.m_fDistance))
		return true;

	return false;
}

bool CCollisionManager::AABBCollision(CollisionInfo& info, BoundingBox bcBox)
{
	// Dynamic to Dynamic

	// Dynamic to Static
	for (auto& staticObject : m_vecStaticMeshContainer) {
		const bool isCollision = staticObject->GetBoundingBox().Intersects(bcBox);
	
		if (isCollision) {
			info.m_pHitObject = staticObject;
			return true;
		}
	}
	return false;
}

bool CCollisionManager::OBBCollision(CollisionInfo& info, BoundingOrientedBox bcObbox)
{
	// Dynamic to Dynamic

	// Dynamic to Static
	for (auto& staticObject : m_vecStaticMeshContainer) {
		const bool isCollision = staticObject->GetBoundingOBox().Intersects(bcObbox);

		if (isCollision) {
			info.m_pHitObject = staticObject;
			return true;
		}
	}
	return false;
}

bool CCollisionManager::CheckCollision(CollisionInfo& info, CGameObject* pOriginObject, CGameObject* pTargetObject)
{
	// 1. Sphere
	if (!pOriginObject->GetBoundingSphere().Intersects(pTargetObject->GetBoundingSphere()))
		return false;

	// 2. OBB			-> static Mesh에 대해서는 AABB 적용하기
	if (!pOriginObject->GetBoundingOBox().Intersects(pTargetObject->GetBoundingOBox()))
		return false;


	//if (pOriginObject->GetMeshTag()) 필요한 객체들만 확인하기
	// 3. Primitive
//	if (!pOriginObject->IsCollision(pTargetObject))
//		return false;

	return true;
}