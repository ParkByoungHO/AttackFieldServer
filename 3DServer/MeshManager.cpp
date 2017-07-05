#include "stdafx.h"
#include "MeshManager.h"


CMeshManager::CMeshManager()
{
}


CMeshManager::~CMeshManager()
{
}



void CMeshManager::InitializeManager()
{
	cout << " ----- Map Data -----" << endl;
	LoadMapData();
}

void CMeshManager::ReleseManager()
{
}


bool CMeshManager::LoadFbxModelDatafromFile(const string& source)
{
	m_fileName = source;
	ifstream fin(source);
	//ifstream fin(fileName, ios::binary);

	if (!fin.is_open())
		return false;

	UINT meshCount = 0;

	string buf;


		fin >> buf; // [FBX_META_DATA]
		fin >> buf >> meshCount;

		// [MESH_DATA]
		LoadMeshData(fin);

		// [BBox_DATA]
		LoadBBoxData(fin);


		// End
		fin >> buf;
	
	fin.close();

	return true;
}

void CMeshManager::AddResourece(const MeshTag& meshTag, const string& source)
{

	cout << "  File Loading < " + source + " > ";

		if (LoadFbxModelDatafromFile(source))
			cout << "\t Success!!" << endl;
		else {
			cout<<("\t Error!! \t 파일 또는 경로를 확인하세요.");
			return;
		}
	

	m_mapBoundingBoxPool.insert(make_pair(meshTag, m_boundingBox));

	// 한 태그에 여러개 등록되었음
	//assert(m_mapBoundingBoxPool.count(meshTag) <= 1);
}




void CMeshManager::LoadMapData()
{

	// ============== Building ============== //
	AddResourece(MeshTag::eBuilding19, "../Assets/FBX Model/Building/Building19/19building.model");
	AddResourece(MeshTag::eBuilding20, "../Assets/FBX Model/Building/Building20/20building.model");
	AddResourece(MeshTag::eBuilding21, "../Assets/FBX Model/Building/Building21/21building.model");
	AddResourece(MeshTag::eBuilding22, "../Assets/FBX Model/Building/Building22/22building.model");
	AddResourece(MeshTag::eBuilding30, "../Assets/FBX Model/Building/Building30/30building.model");
	AddResourece(MeshTag::eBuilding33, "../Assets/FBX Model/Building/Building33/33building.model");
	AddResourece(MeshTag::eBuilding34, "../Assets/FBX Model/Building/Building34/34building.model");
	AddResourece(MeshTag::eBuilding77, "../Assets/FBX Model/Building/Building77/77building.model");
	AddResourece(MeshTag::eBuilding78, "../Assets/FBX Model/Building/Building78/78building.model");
	AddResourece(MeshTag::eBuilding100, "../Assets/FBX Model/Building/Building100/Building100.model");
	AddResourece(MeshTag::eBuilding103, "../Assets/FBX Model/Building/Building103/Building103.model");
	AddResourece(MeshTag::eBuilding104, "../Assets/FBX Model/Building/Building104/Building104.model");

	// Parking Lot
	AddResourece(MeshTag::eParkingLot, "../Assets/FBX Model/ParkingLot/ParkingLot.model");

	// ============== Road ============== //


	AddResourece(MeshTag::eRoad, "../Assets/FBX Model/Road/road.model");
	AddResourece(MeshTag::eCenterRoad, "../Assets/FBX Model/Road/RoadCenter.model");
	AddResourece(MeshTag::eCrossRoad, "../Assets/FBX Model/Road/CrossRoad.model");

	// Etc
	AddResourece(MeshTag::eBench, "../Assets/FBX Model/Bench/Bench.model");

	AddResourece(MeshTag::eBusStop, "../Assets/FBX Model/Busstop/BusStop.model");

	//	AddResourece(TextureTag::eStreetLampD,		"../Assets/FBX Model/StreetLamp/bus_stop_sign.png");
	AddResourece(MeshTag::eStreetLamp, "../Assets/FBX Model/StreetLamp/lamp.model");


	AddResourece(MeshTag::eBarricade, "../Assets/FBX Model/Barricade/streetprops_barricade3.model");


}

void CMeshManager::LoadMeshData(ifstream& fin)
{
	string buf;
	bool						m_bTangent = false;


	UINT						m_nIndexCount = 0;

	UINT						m_nVertexCount = 0;
	vector<XMFLOAT3>			m_vecPosition;
	vector<XMFLOAT3>			m_vecNormal;
	vector<XMFLOAT3>			m_vecTangent;
	vector<XMFLOAT3>			m_vecIndex;
	vector<XMFLOAT2>			m_vecUV;

	UINT						m_nBoneCount = 0;
	UINT						m_nAnimationCount = 0;


	fin >> buf; // [MESH_DATA]
	fin >> m_nVertexCount;
	{
		m_vecPosition.reserve(m_nVertexCount);
		m_vecNormal.reserve(m_nVertexCount);
		if (m_bTangent)
			m_vecTangent.reserve(m_nVertexCount);
		m_vecUV.reserve(m_nVertexCount);
	}
	fin >> m_nIndexCount;
	{
		m_vecIndex.reserve(m_nIndexCount / 3);
	}
	fin >> m_nBoneCount;
	fin >> m_nAnimationCount;

	fin >> buf; // isTangent
	if (buf == "true")
		m_bTangent = true;
	else
		m_bTangent = false;

}

void CMeshManager::LoadBBoxData(ifstream& fin)
{
	string buf;
	XMFLOAT3 bBoxCenter, bBoxExtents;

	fin >> buf; // [BBox_DATA]
	fin >> buf; // Center
	fin >> bBoxCenter.x >> bBoxCenter.y >> bBoxCenter.z;
	fin >> buf; // Extents
	fin >> bBoxExtents.x >> bBoxExtents.y >> bBoxExtents.z;

	m_boundingBox.Center = bBoxCenter;
	m_boundingBox.Extents = bBoxExtents;

}