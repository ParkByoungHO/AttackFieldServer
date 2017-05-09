#pragma once

/*
---------------------------------------------------------------------------------------
싱글톤 템플릿 클레스
---------------------------------------------------------------------------------------
*/

template <typename T>
class CSingletonManager
{
	virtual void ReleseManager() = 0;

protected:
	static T* m_Instance;

	CSingletonManager(){}
	virtual ~CSingletonManager(){};

public:
	static T* GetInstance()
	{
		if (m_Instance == nullptr)
			return m_Instance = new T;
		return m_Instance;
	}

	void ReleseInstance()
	{
		if (m_Instance)
		{
			ReleseManager();
			delete m_Instance;
			m_Instance = nullptr;
		}
	}
	// 추상 클래스 생성
	virtual void InitializeManager() = 0;
	virtual void UpdateManager() {};
};

// 싱글톤 인스턴스 초기화
template <typename T>
T*	CSingletonManager<T>::m_Instance = 0;
