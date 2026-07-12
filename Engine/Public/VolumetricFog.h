#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class ENGINE_DLL CVolumetricFog : public CGameObject {
public:
	DECLARE_DERIVED_TYPE(CVolumetricFog, CGameObject)

private:
	explicit CVolumetricFog();
	explicit CVolumetricFog(const CVolumetricFog& Prototype);
	~CVolumetricFog() override;

public:
	HRESULT Initialize(void* pArg)					override;
	VOID PriorityUpdate(E::_float fTimeDelta)		override;
	VOID Update(E::_float fTimeDelta)				override;
	VOID LateUpdate(E::_float fTimeDelta)			override;
	VOID UpdateGUI()								override;

	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

public:
	static Engine::UPtr<CVolumetricFog> Create();
	Engine::UPtr<CPrototype> Clone(void* pArg)		override;
};

NS_END
