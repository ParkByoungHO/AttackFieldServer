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

enum class ObjectTag
{
	eNone,

	// Road
	eRoad1, eRoad2, eCenterRoad, eCrossRoad,
	eSideWalk1, eSideWalk2,

	// Building
	eBuilding19, eBuilding20, eBuilding21, eBuilding22, eBuilding30, eBuilding33, eBuilding34, eBuilding77, eBuilding78, eBuilding100, eBuilding103, eBuilding104,
	eParkingLot, eHotel, eStoneWall,

	// Etc
	eBench, eGrass, eBusStop, eStreetLamp, eBarricade
};



enum class MeshTag
{
	eNone,

	//enviroment

	// Building
	eBuilding19, eBuilding20, eBuilding21, eBuilding22, eBuilding30, eBuilding33, eBuilding34, eBuilding77, eBuilding78, eBuilding100, eBuilding103, eBuilding104,
	eParkingLot, eHotel,

	// Road
	eRoad, eCenterRoad, eCrossRoad,


	// Etc
	eBench, eBusStop, eStreetLamp, eBarricade,

	// Etc 2
	eTest,
	eBox100m
};