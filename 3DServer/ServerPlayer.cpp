#include "stdafx.h"
#include "ServerPlayer.h"

CServerPlayer::CServerPlayer()
{
	player_move_info.m_d3dxvRight = XMFLOAT3(1.0f, 0.0f, 0.0f);
	player_move_info.m_d3dxvUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
	player_move_info.m_d3dxvLook = XMFLOAT3(0.0f, 0.0f, 1.0f);

	player_move_info.m_fPitch = 0.0f;
	player_move_info.m_fRoll = 0.0f;
	player_move_info.m_fYaw = 0.0f;
}

CServerPlayer::~CServerPlayer()
{
}

void CServerPlayer::UpdateKeyInput(float fTimeElapsed)
{
	// Keyboard
	XMVECTOR d3dxvShift = XMVectorZero();

	if (m_wKeyState & static_cast<int>(KeyInput::eForward)) {
				d3dxvShift += XMLoadFloat3(&player_move_info.m_d3dxvLook);

	}

	if (m_wKeyState & static_cast<int>(KeyInput::eBackward)) {
			d3dxvShift -= XMLoadFloat3(&player_move_info.m_d3dxvLook);

	}

	if (m_wKeyState & static_cast<int>(KeyInput::eLeft)) {
		d3dxvShift -= XMLoadFloat3(&player_move_info.m_d3dxvRight);

	}

	if (m_wKeyState & static_cast<int>(KeyInput::eRight)) {
		d3dxvShift += XMLoadFloat3(&player_move_info.m_d3dxvRight);

	}

	if (m_wKeyState & static_cast<int>(KeyInput::eRun)) {
		d3dxvShift *= 3;		// m_fSpeed 로 변경해야함
		m_Run = true;
	}

	//Mouse
	if (m_wKeyState & static_cast<int>(KeyInput::eLeftMouse)) {
		m_fire = true;

	}
	if (m_wKeyState & static_cast<int>(KeyInput::eRightMouse)) {

	}
	if (m_wKeyState & static_cast<int>(KeyInput::eReload))
	{
		m_Reload = true;
	}

	if (m_wKeyState == 0) {

	}

	d3dxvShift *= player_move_info.m_fSpeed * fTimeElapsed;
	XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + d3dxvShift);

	Move(d3dxvShift);
	Update(0.05);
}

void CServerPlayer::Move(XMVECTOR d3dxvShift)
{
	XMVECTOR d3dxvPosition = XMLoadFloat3(&player_move_info.m_d3dxvPosition) + d3dxvShift;
	XMStoreFloat3(&player_move_info.m_d3dxvPosition, d3dxvPosition);

	
}

void CServerPlayer::Rotate(float x, float y)
{
		XMMATRIX mtxRotate;

			if (x != 0.0f) {
				float fPitch = player_move_info.m_fPitch;
				fPitch += x;
				if (50.0f < fPitch) {
					x -= (fPitch - 50);
					fPitch = 50;
				}
				if (fPitch < -40.0f) {
					x -= (fPitch + 40);
					fPitch = -40;
				}
				player_move_info.m_fPitch = fPitch;
				if (y != 0.0f) {
					float fYaw = player_move_info.m_fYaw;
					fYaw += y;
					if (fYaw > 360.0f) fYaw -= 360.0f;
					if (fYaw < 0.0f) fYaw += 360.0f;
					player_move_info.m_fYaw = fYaw;

					mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&player_move_info.m_d3dxvUp), XMConvertToRadians(y));
					XMStoreFloat3(&player_move_info.m_d3dxvLook, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvLook), mtxRotate));
					XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvRight), mtxRotate));
				}
				
			}
		



		/*if (x != 0.0f)
		{
			player_move_info.m_fPitch += x;
			if (player_move_info.m_fPitch > +50) { x -= (player_move_info.m_fPitch - 50); player_move_info.m_fPitch = +50; }
			if (player_move_info.m_fPitch < -40) { x -= (player_move_info.m_fPitch + 40); player_move_info.m_fPitch = -40; }
		}
		if (y != 0.0f)
		{
			player_move_info.m_fYaw += y;
			if (player_move_info.m_fYaw > 360.0f) player_move_info.m_fYaw -= 360.0f;
			if (player_move_info.m_fYaw < 0.0f) player_move_info.m_fYaw += 360.0f;

			mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&player_move_info.m_d3dxvUp), XMConvertToRadians(y));
			XMStoreFloat3(&player_move_info.m_d3dxvLook, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvLook), mtxRotate));
			XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3TransformNormal(XMLoadFloat3(&player_move_info.m_d3dxvRight), mtxRotate));
		}*/

		XMStoreFloat3(&player_move_info.m_d3dxvLook, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvLook)));
		XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3Cross(XMLoadFloat3(&player_move_info.m_d3dxvUp), XMLoadFloat3(&player_move_info.m_d3dxvLook)));
		XMStoreFloat3(&player_move_info.m_d3dxvRight, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvRight)));
		XMStoreFloat3(&player_move_info.m_d3dxvUp, XMVector3Cross(XMLoadFloat3(&player_move_info.m_d3dxvLook), XMLoadFloat3(&player_move_info.m_d3dxvRight)));
		XMStoreFloat3(&player_move_info.m_d3dxvUp, XMVector3Normalize(XMLoadFloat3(&player_move_info.m_d3dxvUp)));

	


}

void CServerPlayer::Update(float fTimeElapsed)
{
	XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + XMLoadFloat3(&player_move_info.m_d3dxvGravity) * fTimeElapsed);
	float fLength = sqrtf(player_move_info.m_d3dxvVelocity.x * player_move_info.m_d3dxvVelocity.x + player_move_info.m_d3dxvVelocity.z * player_move_info.m_d3dxvVelocity.z);
	float fMaxVelocityXZ = player_move_info.m_fMaxVelocityXZ * fTimeElapsed;

	if (fLength > fMaxVelocityXZ)
	{
		player_move_info.m_d3dxvVelocity.x *= (fMaxVelocityXZ / fLength);
		player_move_info.m_d3dxvVelocity.z *= (fMaxVelocityXZ / fLength);
	}

	// Apply Gravity
	float fMaxVelocityY = player_move_info.m_fMaxVelocityY * fTimeElapsed;
	fLength = sqrtf(player_move_info.m_d3dxvVelocity.y * player_move_info.m_d3dxvVelocity.y);
	if (fLength > fMaxVelocityY) player_move_info.m_d3dxvVelocity.y *= (fMaxVelocityY / fLength);
	//	m_d3dxvVelocity.y = 0;		// 임시 고정

	Move(XMLoadFloat3(&player_move_info.m_d3dxvVelocity));
	//if (m_bIsFloorCollision) OnPlayerUpdated(fTimeElapsed);

	// Apply Deceleration 
	XMVECTOR d3dxvDeceleration = -XMLoadFloat3(&player_move_info.m_d3dxvVelocity);
	d3dxvDeceleration = XMVector3Normalize(d3dxvDeceleration);
	fLength = XMVectorGetX(XMVector3Length(XMLoadFloat3(&player_move_info.m_d3dxvVelocity)));
	float fDeceleration = (player_move_info.m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	XMStoreFloat3(&player_move_info.m_d3dxvVelocity, XMLoadFloat3(&player_move_info.m_d3dxvVelocity) + d3dxvDeceleration * fDeceleration);

	XMFLOAT4X4 mtx;	XMStoreFloat4x4(&mtx, m_mtxWorld);

	mtx._11 = player_move_info.m_d3dxvRight.x;		mtx._12 = player_move_info.m_d3dxvRight.y;		mtx._13 = player_move_info.m_d3dxvRight.z;
	mtx._21 = player_move_info.m_d3dxvUp.x;			mtx._22 = player_move_info.m_d3dxvUp.y;			mtx._23 = player_move_info.m_d3dxvUp.z;
	mtx._31 = player_move_info.m_d3dxvLook.x;		mtx._32 = player_move_info.m_d3dxvLook.y;		mtx._33 = player_move_info.m_d3dxvLook.z;
	mtx._41 = player_move_info.m_d3dxvPosition.x;	mtx._42 = player_move_info.m_d3dxvPosition.y;	mtx._43 = player_move_info.m_d3dxvPosition.z;

	m_mtxWorld = XMLoadFloat4x4(&mtx);

}


void CServerPlayer :: setid(int id) 
{
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