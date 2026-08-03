#pragma once
#include "UITex.h"
#include "Client_Defines.h"
#include <algorithm>
#include "UIObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CHPBar final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(HPBar, E::CUITex)

private:
	CHPBar();
	~CHPBar() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	virtual void PlayEffect(uint32_t uiState);
	void UpdateFill();
private:
	_float s_fAccumulatedTime = 0.f;
	_float m_fCurrentAmount = 1.f;
	_bool m_bDead = false;
private:
	_float m_fMaxFill = 100.f;
	_float m_fcurrentFill = 100.f;
	_float m_fFillDir = 1.f;

public:
	_float GetMaxFill() { return m_fMaxFill; }
	_float GetCurrentFill() { return m_fcurrentFill; }

	void SetMaxFill(_float maxfill) { m_fMaxFill = maxfill; UpdateFill();}
	void SetCurrentFill(_float currentfill) { m_fcurrentFill = currentfill; m_fcurrentFill = std::clamp(m_fcurrentFill, 0.0f, m_fMaxFill);  UpdateFill(); }
	void AddFill(_float addFill) { m_fcurrentFill += addFill; m_fcurrentFill = std::clamp(m_fcurrentFill, 0.0f, m_fMaxFill); UpdateFill(); }
	void SetAmount(_float amount) { m_fCurrentAmount = amount; m_fcurrentFill = m_fMaxFill * amount; UpdateFill(); }
	_float GetAmount() { return m_fCurrentAmount; };


private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

	void PlayMonsterHPDelete(CHandle pHandle);

private:
	CUIObject* SafeGetOBJ(CHandle pHandle)
	{
		if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
			return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);
	}

public:
	static E::UPtr<CHPBar> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

};

NS_END
