#include "stdafx.h"
#include "CharacterObject.h"

CCharacterObject::CCharacterObject()
{
	// 값이 클라이언트에서 변경되면 자동으로 변경되어야 하지만 임시로 설정
	BoundingBox terroristCharacterBoundingBox;
	terroristCharacterBoundingBox.Center = XMFLOAT3(0, -0.1f, 0.2);
	terroristCharacterBoundingBox.Extents = XMFLOAT3(0.6f, 1.5f, 0.55f);

	BoundingBox::CreateMerged(m_bcMeshBoundingBox, m_bcMeshBoundingBox, terroristCharacterBoundingBox);
	BoundingSphere::CreateFromBoundingBox(m_bsMeshBoundingSphere, m_bcMeshBoundingBox);
	BoundingOrientedBox::CreateFromBoundingBox(m_bcMeshBoundingOBox, m_bcMeshBoundingBox);
}

CCharacterObject::~CCharacterObject()
{
}

BoundingOrientedBox CCharacterObject::GetPartsBoundingOBox(UINT index) const
{
	BoundingOrientedBox bcObox = m_bcPartsBoundingOBox[index];
	bcObox.Transform(bcObox, m_mtxPartsBoundingWorld[index]);
	return bcObox;
}

void CCharacterObject::DamagedCharacter(UINT damage)
{
	if (m_nLife <= damage) {
		m_nLife = 0;
		m_bIsDeath = true;
	}
	else
		m_nLife -= damage;
}

void CCharacterObject::OnCollisionCheck()
{	
	/*
	if(IsMoving()){
		for (auto staticObject : COLLISION_MGR->m_vecStaticMeshContainer) {
			CollisionInfo info;
			XMVECTOR velocity = XMLoadFloat3(&m_f3Velocity);

			XMMATRIX d3dxmtxInverse;
			d3dxmtxInverse = XMMatrixInverse(NULL, staticObject->m_mtxWorld);

			XMVECTOR rayPosition;
			rayPosition = XMVector3TransformCoord(GetvPosition(), d3dxmtxInverse);
			
			XMVECTOR rayDirection;
			rayDirection = XMVector3TransformNormal(velocity, d3dxmtxInverse);
			rayDirection = XMVector3Normalize(rayDirection);

			int collisionObjectCount = staticObject->GetMesh()->CheckRayIntersection(&rayPosition, &rayDirection, &info);

			if (collisionObjectCount) {
				if (info.m_fDistance < 4) {
					XMVECTOR slidingVec = velocity - XMVector3Dot(velocity, XMLoadFloat3(&info.m_f3HitNormal)) * XMLoadFloat3(&info.m_f3HitNormal);

					// ----- 슬라이딩 벡터 충돌 ----- //
					XMVECTOR rayDirection;
					rayDirection = XMVector3TransformNormal(slidingVec, d3dxmtxInverse);
					rayDirection = XMVector3Normalize(rayDirection);
					
					CollisionInfo slidingVecInfo;

					int collisionSlidingObjectCount = staticObject->GetMesh()->CheckRayIntersection(&rayPosition, &rayDirection, &slidingVecInfo);
					if (collisionSlidingObjectCount) {
						if (slidingVecInfo.m_fDistance < 2) {
							XMVECTOR normal1 = XMLoadFloat3(&info.m_f3HitNormal);
							XMVECTOR normal2 = XMLoadFloat3(&slidingVecInfo.m_f3HitNormal);
							float fDot = XMVectorGetX(XMVector3Dot(normal1, normal2));

							if (fDot == 0) {			// 직각
								slidingVec = XMVectorSet(0, 0, 0, 0);
							}
							else if (fDot < 0) {		// 예각
								slidingVec = velocity - XMVector3Dot(velocity, XMLoadFloat3(&slidingVecInfo.m_f3HitNormal)) * XMLoadFloat3(&slidingVecInfo.m_f3HitNormal);
							}
							else {						// 둔각
								slidingVec = XMVectorSet(0, 0, 0, 0);
							}
						}
					}
					m_pPlayer->SetvVelocity(slidingVec);	
				}
			}
		}
	}

	// Ground Collision
	XMVECTOR centerPos = m_pPlayer->GetvPrevPosition();			// 혹시 몰라서 이전 프레임의 위치로 설정
	BoundingOrientedBox bcObox = GetBoundingOBox();

	if (COLLISION_MGR->RayCastCollision(m_infoCollision, centerPos, -1 * GetvUp())) {
		if (m_infoCollision.m_fDistance <= bcObox.Extents.y) {
			m_pPlayer->SetFloorCollision(true);
		}
	}
	*/
} 

void CCharacterObject::SetRotate(float fPitch, float fYaw, float fRoll, bool isLocal)
{
	CGameObject::SetRotate(0, fYaw, fRoll, isLocal);
	m_fPitch = fPitch;
}

void CCharacterObject::SetRotate(XMFLOAT3 fAngle, bool isLocal)
{
	CGameObject::SetRotate(0, fAngle.y, fAngle.z, isLocal);
	m_fPitch = fAngle.x;
}

void CCharacterObject::SetRotate(XMVECTOR *pd3dxvAxis, float fAngle, bool isLocal)
{
	XMFLOAT3 axis; XMStoreFloat3(&axis, *pd3dxvAxis);

	CGameObject::SetRotate(0, axis.y, axis.z, isLocal);
	m_fPitch = axis.x;
}

void CCharacterObject::Update(float fDeltaTime)
{ 
	CGameObject::Update(fDeltaTime);
}