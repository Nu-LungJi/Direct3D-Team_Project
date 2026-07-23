

#pragma once
#include "CameraObject.h"

NS_BEGIN(Client)

class CPlayerThirdPersonCamera final : public CCameraObject
{
public:
	struct DESC : public CCameraObject::CAMERA_DESC
	{
		CHandle hTarget{};
		_float fDistance{ 4.5f };
		_float fTargetHeight{};
		_float fPitch{ 15.f };
		_float fMinPitch{ -20.f };
		_float fMaxPitch{ 65.f };
		_float fMouseSensitivity{ 10.f };
	};

public:
	DECLARE_DERIVED_TYPE(CPlayerThirdPersonCamera, CCameraObject)

private:
	CPlayerThirdPersonCamera();
	CPlayerThirdPersonCamera(const CPlayerThirdPersonCamera& rhs);
	~CPlayerThirdPersonCamera() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void UpdateFollow();

private:
	CHandle m_hTarget{};
	_float m_fYaw{};
	_float m_fPitch{ 15.f };
	_float m_fDistance{ 4.5f };
	_float m_fTargetHeight{};
	_float m_fMinPitch{ -20.f };
	_float m_fMaxPitch{ 65.f };
	_float m_fMouseSensitivity{ 10.f };

public:
	static UPtr<CPlayerThirdPersonCamera> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
