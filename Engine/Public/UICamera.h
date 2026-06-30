#pragma once

#include "CameraObject.h"
NS_BEGIN(Engine)

class CUICamera final : public CCameraObject
{
public:
	DECLARE_DERIVED_TYPE(CUICamera, CCameraObject)

protected:
	explicit CUICamera();
	explicit CUICamera(const CUICamera& Prototype);
	~CUICamera() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

public:
	static Engine::UPtr<CUICamera> Create();
	Engine::UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
