#pragma once

#include "SingletonManager.h"

class CMeshManager : public CSingletonManager<CMeshManager>
{
	
	string								m_fileName;
	multimap<MeshTag, BoundingBox>		m_mapBoundingBoxPool;

public:
	CMeshManager();
	virtual ~CMeshManager();

	virtual void InitializeManager() override;
	virtual void ReleseManager() override;

	void LoadMeshData(ifstream& fin);
	void LoadBBoxData(ifstream& fin);

	void LoadMapData();

	BoundingBox GetBoundingBox(const MeshTag& tag)
	{
		return (m_mapBoundingBoxPool.find(tag)->second);
	}


	bool CMeshManager::LoadFbxModelDatafromFile(const string& source);
	void AddResourece(const MeshTag& meshTag, const string& source);
	

	BoundingBox							m_boundingBox;

};

