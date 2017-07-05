#include "stdafx.h"
#include "MapManager.h"
#include "MeshManager.h"

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
}


void CMapManager::InitializeManager()
{
	cout << "============================================================================================" << endl;
	cout << "================================== Map Data Loading ========================================" << endl << endl;

	// Road
	AddMapData(ObjectTag::eRoad1, "MapData/Road1.txt");
	AddMapData(ObjectTag::eRoad2, "MapData/Road2.txt");
	AddMapData(ObjectTag::eCenterRoad, "MapData/Center_Road.txt");
	AddMapData(ObjectTag::eCrossRoad, "MapData/Cross_Road.txt");

	// Building
	AddMapData(ObjectTag::eBuilding19, "MapData/Building19.txt");
	AddMapData(ObjectTag::eBuilding20, "MapData/Building20.txt");
	AddMapData(ObjectTag::eBuilding21, "MapData/Building21.txt");
	AddMapData(ObjectTag::eBuilding22, "MapData/Building22.txt");
	AddMapData(ObjectTag::eBuilding30, "MapData/Building30.txt");
	AddMapData(ObjectTag::eBuilding33, "MapData/Building33.txt");
	AddMapData(ObjectTag::eBuilding34, "MapData/Building34.txt");
	AddMapData(ObjectTag::eBuilding77, "MapData/Building77.txt");
	AddMapData(ObjectTag::eBuilding78, "MapData/Building78.txt");
	AddMapData(ObjectTag::eBuilding100, "MapData/Building100.txt");
	AddMapData(ObjectTag::eBuilding103, "MapData/Building103.txt");
	AddMapData(ObjectTag::eBuilding104, "MapData/Building104.txt");
	AddMapData(ObjectTag::eParkingLot, "MapData/Parking.txt");
	AddMapData(ObjectTag::eHotel, "MapData/Hotel.txt");
	AddMapData(ObjectTag::eBarricade, "MapData/Barricaed.txt");

	// Etc
	AddMapData(ObjectTag::eBusStop, "MapData/Bus_Stop.txt");
	AddMapData(ObjectTag::eStreetLamp, "MapData/Street_lamp.txt");
	AddMapData(ObjectTag::eBench, "MapData/Bench.txt");
	AddMapData(ObjectTag::eGrass, "MapData/Grass.txt");
	AddMapData(ObjectTag::eSideWalk1, "MapData/SideWalk.txt");
	AddMapData(ObjectTag::eSideWalk2, "MapData/SideWalk2.txt");
	AddMapData(ObjectTag::eStoneWall, "MapData/StoneWall1.txt");


	cout << endl;
	cout << "=============================== Map Data Loading Complete ==================================" << endl;
	cout << "============================================================================================" << endl << endl;

	
}




void CMapManager::ReleseManager()
{
}

bool CMapManager::AddMapData(const ObjectTag& tag, const string& source)
{
	ifstream fin(source);
	vector<MapData> vecMapData;
	MapData data;

	cout << "  Map Data File Loading " << source.c_str() ;

	if (!fin.is_open()) {
		cout<<("\t Error!! \t\t 파일 또는 경로를 확인하세요.");
		return false;
	}

	while (!fin.eof()) {
		fin >> data.m_Position.x >> data.m_Position.y >> data.m_Position.z;
		fin >> data.m_Rotation.x >> data.m_Rotation.y >> data.m_Rotation.z;
		fin >> data.m_Scale.x >> data.m_Scale.y >> data.m_Scale.z;

		vecMapData.push_back(data);
	}
	vecMapData.pop_back();
	fin.close();

	m_mapDataPool.insert(make_pair(tag, vecMapData));

	cout<<("\t Success!!")<<endl;
	return true;
}

vector<MapData> CMapManager::GetDataVector(const ObjectTag& tag) const
{
	auto findResource = m_mapDataPool.find(tag);

	if (findResource == m_mapDataPool.end())
		cout << " 끝 " << endl;

	return findResource->second;
}

XMFLOAT3 CMapManager::GetPosition(const ObjectTag& tag, const int index) const
{
	auto findResource = m_mapDataPool.find(tag);

	// Pool에 해당 데이터가 존재하지 않는다.
	assert(findResource != m_mapDataPool.end());

	return findResource->second[index].m_Position;
}

XMFLOAT3 CMapManager::GetRotation(const ObjectTag& tag, const int index) const
{
	auto findResource = m_mapDataPool.find(tag);

	// Pool에 해당 데이터가 존재하지 않는다.
	assert(findResource != m_mapDataPool.end());

	return findResource->second[index].m_Rotation;
}

XMFLOAT3 CMapManager::GetScale(const ObjectTag& tag, const int index) const
{
	auto findResource = m_mapDataPool.find(tag);

	// Pool에 해당 데이터가 존재하지 않는다.
	assert(findResource != m_mapDataPool.end());

	return findResource->second[index].m_Scale;
}