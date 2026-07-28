#pragma once
#include "CameraObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCinematicCamera final : public CCameraObject
{
public:
	DECLARE_DERIVED_TYPE(CCinematicCamera, CCameraObject)

protected:
	CCinematicCamera();
	CCinematicCamera(const CCinematicCamera& Prototype);
	~CCinematicCamera() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;

public:
	virtual _bool IsPersistent() const { return true; }
public:
	static UPtr<CCinematicCamera> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END
