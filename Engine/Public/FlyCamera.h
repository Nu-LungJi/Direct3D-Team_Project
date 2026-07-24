#pragma once
#include "CameraObject.h"
NS_BEGIN(Engine)
class ENGINE_DLL CFlyCamera final : public CCameraObject
{
public:
	DECLARE_DERIVED_TYPE(CFlyCamera, CCameraObject)


public:
	void UpdateGUI() override;

protected:
	explicit CFlyCamera();
	explicit CFlyCamera(const CFlyCamera& Prototype);
	~CFlyCamera() override;

public:
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
 
private:
	_bool m_bFix{ false };

private:
	uint32_t m_iColliderIntersect{};

public:
	static Engine::UPtr<CFlyCamera> Create();
	Engine::UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
