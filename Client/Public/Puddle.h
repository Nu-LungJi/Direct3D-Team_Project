#pragma once
#include "DecalVolume.h"

NS_BEGIN(Client)

class CPuddle final : public CDelVolume {
public:
	DECLARE_DERIVED_TYPE(CPuddle, CGameObject)

private:
	CPuddle();
	CPuddle(const CPuddle& prototype);
	~CPuddle() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	static E::UPtr<CPuddle> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
