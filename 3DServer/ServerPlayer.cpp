#include "ServerPlayer.h"

inline D3DXVECTOR3&& D3DXLoadFLOAT3(const XMFLOAT3 xmf)
{
	return D3DXVECTOR3(xmf.x, xmf.y, xmf.z);
}

CServerPlayer::CServerPlayer()
{
	player_move_info.m_d3dxvRight = XMFLOAT3(1.0f, 0.0f, 0.0f);
	player_move_info.m_d3dxvUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
	player_move_info.m_d3dxvLook = XMFLOAT3(0.0f, 0.0f, 1.0f);
}


CServerPlayer::~CServerPlayer()
{
}

void CServerPlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{

	{
		if (dwDirection)
		{
			XMVECTOR d3dxvShift = XMVectorZero();
			if (dwDirection & DIR_FORWARD) d3dxvShift += XMLoadFloat3(&player_move_info.m_d3dxvLook) * fDistance;
			if (dwDirection & DIR_BACKWARD) d3dxvShift -= XMLoadFloat3(&player_move_info.m_d3dxvLook) * fDistance;
			if (dwDirection & DIR_RIGHT) d3dxvShift += XMLoadFloat3(&player_move_info.m_d3dxvRight) * fDistance;
			if (dwDirection & DIR_LEFT) d3dxvShift -= XMLoadFloat3(&player_move_info.m_d3dxvRight) * fDistance;
			if (dwDirection & DIR_UP) d3dxvShift += XMLoadFloat3(&player_move_info.m_d3dxvUp) * fDistance;
			if (dwDirection & DIR_DOWN) d3dxvShift -= XMLoadFloat3(&player_move_info.m_d3dxvUp) * fDistance;

			Move(d3dxvShift, bUpdateVelocity);

			//현재방향 저장
			player_move_info.dwDirection = dwDirection;
		}
	}
}

void CServerPlayer::Move(const XMVECTOR& d3dxvShift, bool bUpdateVelocity)
{

	{
		if (bUpdateVelocity)
		{
			XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + d3dxvShift);
		}
		else
		{
			XMVECTOR d3dxvPosition = XMLoadFloat3(&player_move_info.m_d3dxvPosition);
			d3dxvPosition = d3dxvPosition + d3dxvShift;
			XMStoreFloat3(&player_move_info.m_d3dxvPosition, d3dxvPosition);
		}
	}
}


void CServerPlayer::Rotate(float x, float y, float z)
{

	{
		XMMATRIX mtxRotate;

		if (x != 0.0f)
		{
			player_move_info.m_fPitch += x;
			if (player_move_info.m_fPitch > +89.0f) { x -= (player_move_info.m_fPitch - 89.0f); player_move_info.m_fPitch = +89.0f; }
			if (player_move_info.m_fPitch < -89.0f) { x -= (player_move_info.m_fPitch + 89.0f); player_move_info.m_fPitch = -89.0f; }
		}
		if (y != 0.0f)
		{
			player_move_info.m_fYaw += y;
			if (player_move_info.m_fYaw > 360.0f) player_move_info.m_fYaw -= 360.0f;
			if (player_move_info.m_fYaw < 0.0f) player_move_info.m_fYaw += 360.0f;
		}
		if (z != 0.0f)
		{
			player_move_info.m_fRoll += z;
			if (player_move_info.m_fRoll > +20.0f) { z -= (player_move_info.m_fRoll - 20.0f); player_move_info.m_fRoll = +20.0f; }
			if (player_move_info.m_fRoll < -20.0f) { z -= (player_move_info.m_fRoll + 20.0f); player_move_info.m_fRoll = -20.0f; }
		}

		if (y != 0.0f)
		{
			mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&player_move_info.m_d3dxvUp), XMConvertToRadians(y));
			XMStoreFloat3(&player_move_info.m_d3dxvLook, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvLook), mtxRotate));
			XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvRight), mtxRotate));
		}

		XMStoreFloat3(&player_move_info.m_d3dxvLook, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvLook)));
		XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3Cross(XMLoadFloat3(&player_move_info.m_d3dxvUp), XMLoadFloat3(&player_move_info.m_d3dxvLook)));
		XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvRight)));
		XMStoreFloat3(&player_move_info.m_d3dxvUp, XMVector3Cross(XMLoadFloat3(&player_move_info.m_d3dxvLook), XMLoadFloat3(&player_move_info.m_d3dxvRight)));
		XMStoreFloat3(&player_move_info.m_d3dxvUp, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvUp)));

	}
}

void CServerPlayer::Update(float fTimeElapsed)
{

	{
		XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + XMLoadFloat3(&player_move_info.m_d3dxvGravity) * fTimeElapsed);
		float fLength = sqrtf(player_move_info.m_d3dxvVelocity.x * player_move_info.m_d3dxvVelocity.x + player_move_info.m_d3dxvVelocity.z * player_move_info.m_d3dxvVelocity.z);
		float fMaxVelocityXZ = player_move_info.m_fMaxVelocityXZ * fTimeElapsed;
		if (fLength > fMaxVelocityXZ)
		{
			player_move_info.m_d3dxvVelocity.x *= (fMaxVelocityXZ / fLength);
			player_move_info.m_d3dxvVelocity.z *= (fMaxVelocityXZ / fLength);
		}
		float fMaxVelocityY = player_move_info.m_fMaxVelocityY * fTimeElapsed;
		fLength = sqrtf(player_move_info.m_d3dxvVelocity.y * player_move_info.m_d3dxvVelocity.y);
		if (fLength > fMaxVelocityY) player_move_info.m_d3dxvVelocity.y *= (fMaxVelocityY / fLength);
		Move(XMLoadFloat3(&player_move_info.m_d3dxvVelocity), false);



		XMVECTOR d3dxvDeceleration = -XMLoadFloat3(&player_move_info.m_d3dxvVelocity);
		d3dxvDeceleration = XMVector3Normalize(d3dxvDeceleration);

		D3DXVECTOR3 v3 = D3DXLoadFLOAT3(player_move_info.m_d3dxvVelocity);
		fLength = D3DXVec3Length(&v3);
		float fDeceleration = (player_move_info.m_fFriction * fTimeElapsed);
		if (fDeceleration > fLength) fDeceleration = fLength;
		XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + d3dxvDeceleration * fDeceleration);
	}
}