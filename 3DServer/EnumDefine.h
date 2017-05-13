#pragma once


enum class KeyInput
{
	eNone,

	// Direction & Moving
	eForward = 0x01,
	eBackward = 0x02,
	eLeft = 0x04,
	eRight = 0x08,
	eRun = 0x10,
	eReload = 0x20,

	// Mouse
	eLeftMouse = 0x100,
	eRightMouse = 0x40,
	eMouseWheel = 0x80,		// 휠 위아래는 추후 구현

};

enum class ChracterBoundingBoxParts
{
	eNone = -1,
	eBody, eHead,
	eLeftUpArm, eLeftDownArm, eRightUpArm, eRightDownArm,
	eLeftUpLeg, eLeftDownLeg, eRightUpLeg, eRightDownLeg,

	ePartsCount
};

enum class TeamType
{
	eNone,
	eRedTeam, eBlueTeam
};