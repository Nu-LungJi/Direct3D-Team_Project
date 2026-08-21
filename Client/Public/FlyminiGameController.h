#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)

class CHandle;

NS_END

NS_BEGIN(Client)

class CFlightRing;

class CFlyminiGameController final : public CGameObject
{
public:
	struct CFlyminiGameController_DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hPlayer;
	};
public:
	DECLARE_DERIVED_TYPE(CFlyminiGameController, CGameObject)

private:
	CFlyminiGameController();
	~CFlyminiGameController() override;

public:
	void UpdateGUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

private:
	std::vector<CHandle> m_vecFlightRing;
	CHandle m_hPlayer;

	_float m_fTotalTime = 0.f;
	_float m_fCurTime = 0.f;

	uint32_t m_iTotalRing = 0.f;
	uint32_t m_iPassRing = 0.f;

public:
	static E::UPtr<CFlyminiGameController> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
