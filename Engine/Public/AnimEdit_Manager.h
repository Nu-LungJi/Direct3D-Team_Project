
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
	uint32_t GetAnimIndex();
public:
	void IMGUI_Select_AnimType();
	void IMGUI_Slider_Animation();
	void IMGUI_Select_Animation();
public:
	void IMGUI_TestGetAnimIndex();
	void UpdateGUI();
private:
	CHandle m_hTestModel{};
	_float	m_fTimeDelta{ 0.f };


public:
	static UPtr<CAnimEdit_Manager> Create();
};

NS_END