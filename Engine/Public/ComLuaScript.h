#pragma once
#include "Component.h"
#include "ResLuaScript.h"
#include "LuaEnv.h"
NS_BEGIN(Engine)

class ENGINE_DLL CComLuaScript : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
		SPtr<CResLuaScript> pResScript{};
		std::function<void(CComLuaScript*)> funcScriptLoad{};
		std::function<void(CComLuaScript*)> funcScriptCreate{};
	};

public:
	DECLARE_DERIVED_TYPE(CComLuaScript, CComponent)

public:
	void UpdateGUI() override;

protected:
	explicit CComLuaScript();
	~CComLuaScript() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);

protected:
	sol::protected_function m_OnCreate;
	sol::protected_function m_OnDestroy;
//
	sol::protected_function m_PriorityUpdate;
	sol::protected_function m_FixedUpdate;
	sol::protected_function m_Update;
	sol::protected_function m_LateUpdate;

protected:
	std::function<void(CComLuaScript*)> m_funcScriptLoad{};
	std::function<void(CComLuaScript*)> m_funcScriptCreate{};

public:
	static UPtr<CComLuaScript> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

public:
	sol::environment& GetEnv() { return m_pLuaEnv->GetEnv(); }

protected:
	SPtr<CLuaEnv> m_pLuaEnv{};

protected:
	void Free() override;
};

NS_END
