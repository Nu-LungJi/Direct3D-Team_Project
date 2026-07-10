#pragma once
#include "Component.h"
#include "ResLuaScript.h"
NS_BEGIN(Engine)

class ENGINE_DLL CComLuaScript : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		SPtr<CResLuaScript> pResScript{};
	};

public:
	DECLARE_DERIVED_TYPE(CComLuaScript, CComponent)

public:
	void UpdateGUI() override;

private:
	explicit CComLuaScript();
	~CComLuaScript() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);

private:
	void CacheFunctions(_string_view name, sol::protected_function& out);
	sol::protected_function m_OnCreate;
	sol::protected_function m_OnDestroy;

	sol::protected_function m_PriorityUpdate;
	sol::protected_function m_FixedUpdate;
	sol::protected_function m_Update;
	sol::protected_function m_LateUpdate;

public:
	static UPtr<CComLuaScript> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResLuaScript> m_pResLuaScript{};
	sol::environment m_Environment;
};

NS_END
