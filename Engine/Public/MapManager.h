#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CMapManager : public CEngineBase
{
public:
	CMapManager(const CMapManager&) = delete;
	CMapManager& operator=(const CMapManager& rhs) = delete;

private:
	CMapManager();
	~CMapManager() override;

private:
	HRESULT Initialize();

public:
	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

public:
	void UpdateGUI();

public:
	HRESULT SaveMap(const std::string& path);
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true);


//public:
//	void FrameStart();
//	void FrameEnd();

public:
	static UPtr<CMapManager> Create();

public:
	void Free() override;

};

NS_END

