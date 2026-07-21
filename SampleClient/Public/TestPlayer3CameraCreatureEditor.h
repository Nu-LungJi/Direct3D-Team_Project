#pragma once
#include "CameraObject.h"

NS_BEGIN(Engine)
class CCollider;
class CCollFrustum;
NS_END

NS_BEGIN(Client)

class CTestPlayer3CameraCreatureEditor final : public CCameraObject
{
public:
	/*
	hTarget          추적할 플레이어
	fDistance        플레이어와 카메라 사이 거리
	fTargetHeight    카메라가 바라볼 플레이어 높이
	fPitch           초기 상하 각도
	fMinPitch        최소 상하 각도
	fMaxPitch        최대 상하 각도
	fMouseSensitivity 마우스 회전 감도
	*/
	struct DESC : public CCameraObject::CAMERA_DESC
	{
		CHandle hTarget{};
		_float fDistance{ 5.f };
		_float fTargetHeight{ 0.5f };
		_float fPitch{ 15.f };
		_float fMinPitch{ -20.f };
		_float fMaxPitch{ 65.f };
		_float fMouseSensitivity{ 10.f };
	};

public:
	DECLARE_DERIVED_TYPE(CTestPlayer3CameraCreatureEditor, CCameraObject)

private:
	CTestPlayer3CameraCreatureEditor();
	CTestPlayer3CameraCreatureEditor(const CTestPlayer3CameraCreatureEditor& rhs);
	~CTestPlayer3CameraCreatureEditor() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateFollow();
	const CCollFrustum* GetFrustumCollider() const;

private:
	CHandle m_hTarget{};
	_float m_fYaw{};
	_float m_fPitch{ 15.f };
	_float m_fDistance{ 5.f };
	_float m_fTargetHeight{ 0.5f };
	_float m_fMinPitch{ -20.f };
	_float m_fMaxPitch{ 65.f };
	_float m_fMouseSensitivity{ 10.f };
	UPtr<CCollider> m_pFrustumCollider{};

public:
	static UPtr<CTestPlayer3CameraCreatureEditor> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
