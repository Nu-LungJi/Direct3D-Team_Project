
#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;

class CAnimEdit_Manager final : public CEngineBase
{
public:


private:
	CAnimEdit_Manager();
	~CAnimEdit_Manager();

public:
	HRESULT Initilize();


	HRESULT SetupTestModel();
public:

	void Update(_float fTimeDelta);

	void SetTestModelHandle(const CHandle& handle) { m_hTestModel = handle; }

public:
	void IMGUI_Select_AnimType();
	void IMGUI_Slider_Animation();
public:
	void UpdateGUI();
private:
	CHandle m_hTestModel{};

public:
	static UPtr<CAnimEdit_Manager> Create();
};

NS_END