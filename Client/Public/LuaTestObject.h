#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComLuaScript;
NS_END

NS_BEGIN(Client)

class CLuaTestObject final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CLuaTestObject, CGameObject)

private:
	CLuaTestObject();
	CLuaTestObject(const CLuaTestObject& Prototype);
	~CLuaTestObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;

private:
	CComLuaScript* m_pComLuaScript{};

public:
	static UPtr<CLuaTestObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
