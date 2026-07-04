#pragma once

#include "UIObject.h"
#include "Client_Defines.h"
NS_BEGIN(Client)

class CTexUI final : public E::CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CTexUI, CUIObject)

private:
	CTexUI();
	~CTexUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	std::string Get_ResTag() { return m_sRestag; }
	void Set_ResTag(std::string tag) { m_sRestag = tag; }
	void SetMouseTracking(_bool isTracking) { m_bMouseTracking = isTracking; }
private:
	std::string m_sRestag;
	_bool m_bMouseTracking{};

private:
	// 쉐이더
	bool m_bOutline{};

public:
	static E::UPtr<CTexUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

};

NS_END