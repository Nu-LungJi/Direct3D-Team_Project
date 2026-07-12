#pragma once
#include "CameraObject.h"
NS_BEGIN(Engine)

class CShadowCamera final : public CCameraObject
{
public:
	DECLARE_DERIVED_TYPE(CFlyCamera, CCameraObject)

public:
	void UpdateGUI() override;

protected:
	explicit CShadowCamera();
	explicit CShadowCamera(const CShadowCamera& Prototype);
	~CShadowCamera() override;

public:
	HRESULT Initialize(void* pArg)				override;
	void PriorityUpdate(E::_float fTimeDelta)	override;
	void Update(E::_float fTimeDelta)			override;
	void LateUpdate(E::_float fTimeDelta)		override;

public:
	static Engine::UPtr<CShadowCamera> Create();
	Engine::UPtr<CPrototype> Clone(void* pArg) override;
};
NS_END
