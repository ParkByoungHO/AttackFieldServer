//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"

///////////////////////////////////////////////////////////////////////////

UINT			CGameObject::g_nObjectId = 0;

CGameObject::CGameObject(int nMeshes)
{
	m_bcMeshBoundingBox.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_bcMeshBoundingBox.Extents = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_nObjectId = ++g_nObjectId;
}

CGameObject::~CGameObject()
{
}

void CGameObject::GenerateRayForPicking(XMVECTOR *pd3dxvPickPosition, XMMATRIX *pd3dxmtxWorld, XMMATRIX *pd3dxmtxView, XMVECTOR *pd3dxvPickRayPosition, XMVECTOR *pd3dxvPickRayDirection)
{
	XMMATRIX d3dxmtxInverse;
	XMMATRIX d3dxmtxWorldView = *pd3dxmtxView;
	if (pd3dxmtxWorld) 
		d3dxmtxWorldView = XMMatrixMultiply(*pd3dxmtxWorld, *pd3dxmtxView);
	d3dxmtxInverse = XMMatrixInverse(NULL, d3dxmtxWorldView);
	
	XMVECTOR d3dxvCameraOrigin = XMVectorZero();
	*pd3dxvPickRayPosition = XMVector3TransformCoord(d3dxvCameraOrigin, d3dxmtxInverse);
	*pd3dxvPickRayDirection = XMVector3TransformCoord(*pd3dxvPickPosition, d3dxmtxInverse);
	*pd3dxvPickRayDirection = *pd3dxvPickRayDirection - *pd3dxvPickRayPosition;
}

void CGameObject::SetPosition(float x, float y, float z, bool isLocal)
{
	XMFLOAT4X4 mtx; 
	if (isLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._41 = x;  mtx._42 = y; 	mtx._43 = z;
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._41 = x;  mtx._42 = y; 	mtx._43 = z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::SetPosition(XMVECTOR d3dxvPosition, bool isLocal)
{
	XMFLOAT4 f4vPosition;
	XMStoreFloat4(&f4vPosition, d3dxvPosition);
	SetPosition(f4vPosition.x, f4vPosition.y, f4vPosition.z, isLocal);
}

void CGameObject::SetPosition(XMFLOAT3 pos, bool isLocal)
{
	XMFLOAT4X4 mtx;
	if (isLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._41 = pos.x; mtx._42 = pos.y; mtx._43 = pos.z;
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._41 = pos.x; mtx._42 = pos.y; mtx._43 = pos.z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::MoveStrafe(float fDistance)
{
	XMVECTOR d3dxvPosition = GetvPosition();
	XMVECTOR d3dxvRight = GetvRight();
	d3dxvPosition += fDistance * d3dxvRight;
	CGameObject::SetPosition(d3dxvPosition);
}

void CGameObject::MoveUp(float fDistance)
{
	XMVECTOR d3dxvPosition = GetvPosition();
	XMVECTOR d3dxvUp = GetvUp();
	d3dxvPosition += fDistance * d3dxvUp;
	CGameObject::SetPosition(d3dxvPosition);
}

void CGameObject::MoveForward(float fDistance)
{
	XMVECTOR d3dxvPosition = GetvPosition();
	XMVECTOR d3dxvLookAt = GetvLook();
	d3dxvPosition += fDistance * d3dxvLookAt;
	CGameObject::SetPosition(d3dxvPosition);
}

void CGameObject::Move(XMFLOAT3 vPos, bool isLocal)
{
	if (isLocal) {
		XMVECTOR d3dxvPosition = GetvPosition(true) + XMLoadFloat3(&vPos);
		CGameObject::SetPosition(d3dxvPosition, true);
	}
	else {
		XMVECTOR d3dxvPosition = GetvPosition() + XMLoadFloat3(&vPos);
		CGameObject::SetPosition(d3dxvPosition);
	}
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll, bool isLocal)
{
	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), 
		XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));

	if (isLocal)
		m_mtxLocal = mtxRotate * m_mtxLocal;
	else
		m_mtxWorld = mtxRotate * m_mtxWorld;
}

void CGameObject::Rotate(XMFLOAT3 fAngle, bool isLocal)
{
	if (fAngle.x >= 360)
		fAngle.x = fAngle.x - 360;
	if (fAngle.y >= 360)
		fAngle.y = fAngle.y - 360;
	if (fAngle.z >= 360)
		fAngle.z = fAngle.z - 360;



	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fAngle.x),
		XMConvertToRadians(fAngle.y), XMConvertToRadians(fAngle.z));

	if (isLocal)
		m_mtxLocal = mtxRotate * m_mtxLocal;
	else
		m_mtxWorld = mtxRotate * m_mtxWorld;
}

void CGameObject::Rotate(XMVECTOR *pd3dxvAxis, float fAngle, bool isLocal)
{
	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationAxis(*pd3dxvAxis, XMConvertToRadians(fAngle));

	if (isLocal)
		m_mtxLocal = mtxRotate * m_mtxLocal;
	else
		m_mtxWorld = mtxRotate * m_mtxWorld;
}

void CGameObject::SetRotate(float fPitch, float fYaw, float fRoll, bool isLocal)
{
	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch),
		XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));

	XMFLOAT4X4 mtx; XMStoreFloat4x4(&mtx, mtxRotate);
	XMFLOAT3 position = GetPosition(isLocal);
	
	mtx._41 = position.x;
	mtx._42 = position.y;
	mtx._43 = position.z;

	if (isLocal)
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	else
		m_mtxWorld = XMLoadFloat4x4(&mtx);
}

void CGameObject::SetRotate(XMFLOAT3 fAngle, bool isLocal)
{
	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fAngle.x),
		XMConvertToRadians(fAngle.y), XMConvertToRadians(fAngle.z));

	XMFLOAT4X4 mtx; XMStoreFloat4x4(&mtx, mtxRotate);
	XMFLOAT3 position = GetPosition(isLocal);

	mtx._41 = position.x;
	mtx._42 = position.y;
	mtx._43 = position.z;

	if (isLocal)
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	else
		m_mtxWorld = XMLoadFloat4x4(&mtx);
}

void CGameObject::SetRotate(XMVECTOR *pd3dxvAxis, float fAngle, bool isLocal)
{
	XMMATRIX mtxRotate;
	mtxRotate = XMMatrixRotationAxis(*pd3dxvAxis, XMConvertToRadians(fAngle));

	XMFLOAT4X4 mtx; XMStoreFloat4x4(&mtx, mtxRotate);
	XMFLOAT3 position = GetPosition(isLocal);

	mtx._41 = position.x;
	mtx._42 = position.y;
	mtx._43 = position.z;

	if (isLocal)
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	else
		m_mtxWorld = XMLoadFloat4x4(&mtx);
}

XMVECTOR CGameObject::GetvPosition(bool isLocal) const
{
	XMFLOAT4X4 mtx;
	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else 
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	return XMVectorSet(mtx._41, mtx._42, mtx._43, 0.f);
}

XMFLOAT3 CGameObject::GetPosition(bool isLocal) const
{
	XMFLOAT4X4 mtx;
	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	return XMFLOAT3(mtx._41, mtx._42, mtx._43);
}

XMVECTOR CGameObject::GetvRight(bool isLocal) const
{
	XMFLOAT4X4 mtx;
	
	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvRight = XMVectorSet(mtx._11, mtx._12, mtx._13, 0.f);
	d3dxvRight = XMVector3NormalizeEst(d3dxvRight);

	return d3dxvRight;
}

XMVECTOR CGameObject::GetvUp(bool isLocal) const
{
	XMFLOAT4X4 mtx;

	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvUp = XMVectorSet(mtx._21, mtx._22, mtx._23, 0.f);
	d3dxvUp = XMVector3Normalize(d3dxvUp);

	return d3dxvUp;
}

XMVECTOR CGameObject::GetvLook(bool isLocal) const
{
	XMFLOAT4X4 mtx;

	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvLookAt = XMVectorSet(mtx._31, mtx._32, mtx._33, 0.f);
	d3dxvLookAt = XMVector3Normalize(d3dxvLookAt);

	return d3dxvLookAt;
}

XMFLOAT3 CGameObject::GetRight(bool isLocal) const
{
	XMFLOAT4X4 mtx;

	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvRight = XMVectorSet(mtx._11, mtx._12, mtx._13, 0.f);
	d3dxvRight = XMVector3NormalizeEst(d3dxvRight);

	XMFLOAT3 right; XMStoreFloat3(&right, d3dxvRight);
	return right;
}

XMFLOAT3 CGameObject::GetUp(bool isLocal) const
{
	XMFLOAT4X4 mtx;

	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvUp = XMVectorSet(mtx._21, mtx._22, mtx._23, 0.f);
	d3dxvUp = XMVector3Normalize(d3dxvUp);

	XMFLOAT3 up; XMStoreFloat3(&up, d3dxvUp);
	return up;
}

XMFLOAT3 CGameObject::GetLook(bool isLocal) const
{
	XMFLOAT4X4 mtx;

	if (isLocal)
		XMStoreFloat4x4(&mtx, m_mtxLocal);
	else
		XMStoreFloat4x4(&mtx, m_mtxWorld);

	XMVECTOR d3dxvLookAt = XMVectorSet(mtx._31, mtx._32, mtx._33, 0.f);
	d3dxvLookAt = XMVector3Normalize(d3dxvLookAt);

	XMFLOAT3 look; XMStoreFloat3(&look, d3dxvLookAt);
	return look;
}

BoundingBox CGameObject::GetBoundingBox(bool isLocal) const
{
	if (isLocal) 
		return m_bcMeshBoundingBox;
	else {
		BoundingBox bcBox = m_bcMeshBoundingBox;
		bcBox.Transform(bcBox, m_mtxWorld);
		return bcBox;
	}
}

BoundingOrientedBox CGameObject::GetBoundingOBox(bool isLocal) const
{
	if (isLocal)
		return m_bcMeshBoundingOBox;
	else {
		BoundingOrientedBox bcObox = m_bcMeshBoundingOBox;
		bcObox.Transform(bcObox, m_mtxWorld);
		return bcObox;
	}
}

BoundingSphere CGameObject::GetBoundingSphere(bool isLocal) const
{
	if (isLocal)
		return m_bsMeshBoundingSphere;
	else {
		BoundingSphere bsSphere = m_bsMeshBoundingSphere;
		bsSphere.Transform(bsSphere, m_mtxWorld);
		return bsSphere;
	}
}

void CGameObject::SetRight(XMFLOAT3 axis, bool isLocal)
{
	XMFLOAT4X4 mtx;

	if (isLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._11 = axis.x, mtx._12 = axis.y, mtx._13 = axis.z;

		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._11 = axis.x, mtx._12 = axis.y, mtx._13 = axis.z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::SetUp(XMFLOAT3 axis, bool isLocal)
{
	XMFLOAT4X4 mtx;

	if (isLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._21 = axis.x, mtx._22 = axis.y, mtx._23 = axis.z;

		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._21 = axis.x, mtx._22 = axis.y, mtx._23 = axis.z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::SetLook(XMFLOAT3 axis, bool isLocal)
{
	XMFLOAT4X4 mtx;
	
	if (isLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._31 = axis.x, mtx._32 = axis.y, mtx._33 = axis.z;

		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._31 = axis.x, mtx._32 = axis.y, mtx._33 = axis.z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::SetvLook(XMVECTOR axis, bool bIsLocal)
{
	XMFLOAT4X4 mtx;
	XMFLOAT3 look; XMStoreFloat3(&look, axis);

	if (bIsLocal) {
		XMStoreFloat4x4(&mtx, m_mtxLocal);
		mtx._31 = look.x, mtx._32 = look.y, mtx._33 = look.z;
		m_mtxLocal = XMLoadFloat4x4(&mtx);
	}
	else {
		XMStoreFloat4x4(&mtx, m_mtxWorld);
		mtx._31 = look.x, mtx._32 = look.y, mtx._33 = look.z;
		m_mtxWorld = XMLoadFloat4x4(&mtx);
	}
}

void CGameObject::Update(float fDeltaTime)
{
	if (!XMMatrixIsIdentity(m_mtxLocal))
		m_mtxWorld = XMMatrixMultiply(m_mtxLocal, m_mtxWorld);
}